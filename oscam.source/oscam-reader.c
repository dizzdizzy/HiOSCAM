#define MODULE_LOG_PREFIX "reader"

#include "globals.h"
#include <sys/system_properties.h>
#include <net/if.h>
#include "cscrypt/md5.h"
#include "module-cccam.h"
#include "module-led.h"
#include "module-stat.h"
#include "module-dvbapi.h"
#if defined(MODULE_CONSTCW)
#include "module-constcw.h"
#endif
#include "oscam-config.h"
#include "oscam-conf.h"
#include "oscam-files.h"
#include "oscam-cache.h"
#include "oscam-chk.h"
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-garbage.h"
#include "oscam-lock.h"
#include "oscam-net.h"
#include "oscam-reader.h"
#include "oscam-string.h"
#include "oscam-time.h"
#include "oscam-work.h"
#include "reader-common.h"

#if 1
	#define	MYREADER_TRACE			myprintf
#else
	#define	MYREADER_TRACE(...)
#endif
/*
 caid	= 0624
 rsakey= 79EA25A763DA2C3E02B456A13962E60BCE63E628A2C177BE951CED96A9C6131A146F98D5867B7AE6682324FD6481913C0241F065C8D3457E54BB59B7B5DE0362
 boxkey= A1C6F3D8B5E1F2C1
 */

extern CS_MUTEX_LOCK system_lock;
extern CS_MUTEX_LOCK ecmcache_lock;
extern struct ecm_request_t *ecmcwcache;
extern const struct s_cardsystem *cardsystems[];
// sky(n)
extern char  *entitlement_type[];
extern struct s_client	*dvbApi_client;

const char *RDR_CD_TXT[] = {
									"cd", "dsr", "cts", "ring", "none",
									"gpio1", "gpio2", "gpio3", "gpio4", "gpio5", "gpio6", "gpio7",
									NULL
};

/* Overide ratelimit priority for dvbapi request */
static int32_t dvbapi_override_prio(struct s_reader *reader, ECM_REQUEST *er, int32_t maxecms, struct timeb *actualtime)
{
	if (!module_dvbapi_enabled() || !is_dvbapi_usr(er->client->account->usr))
		return -1;

	int32_t foundspace = -1;
	int64_t gone = 0;

	if (reader->lastdvbapirateoverride.time == 0) // fixup for first run!
		gone = comp_timeb(actualtime, &reader->lastdvbapirateoverride);

	if (gone > reader->ratelimittime) {
		int32_t h;
		struct timeb minecmtime = *actualtime;
		for (h = 0; h < MAXECMRATELIMIT; h++) {
			gone = comp_timeb(&minecmtime, &reader->rlecmh[h].last);
			if (gone > 0) {
				minecmtime = reader->rlecmh[h].last;
				foundspace = h;
			}
		}
		reader->lastdvbapirateoverride = *actualtime;
		cs_log_dbg(D_CLIENT, "prioritizing DVBAPI user %s over other watching client", er->client->account->usr);
		cs_log_dbg(D_CLIENT, "ratelimiter forcing srvid %04X into slot %d/%d of reader %s", er->srvid, foundspace + 1, maxecms, reader->label);
	} else {
		cs_log_dbg(D_CLIENT, "DVBAPI User %s is switching too fast for ratelimit and can't be prioritized!",
					   er->client->account->usr);
	}
	return foundspace;
}

static int32_t ecm_ratelimit_findspace(struct s_reader * reader, ECM_REQUEST *er, struct ecmrl rl, int32_t reader_mode)
{

	int32_t h, foundspace = -1;
	int32_t maxecms = MAXECMRATELIMIT; // init maxecms
	int32_t totalecms = 0; // init totalecms
	struct timeb actualtime;
	cs_ftime(&actualtime);
	for (h = 0; h < MAXECMRATELIMIT; h++) // release slots with srvid that are overtime, even if not called from reader module to maximize available slots!
	{
		if (reader->rlecmh[h].last.time == -1) { continue; }
		int64_t gone = comp_timeb(&actualtime, &reader->rlecmh[h].last);
		if (gone >= (reader->rlecmh[h].ratelimittime + reader->rlecmh[h].srvidholdtime) || gone < 0) // gone <0 fixup for bad systemtime on dvb receivers while changing transponders
		{
			cs_log_dbg(D_CLIENT, "ratelimiter srvid %04X released from slot %d/%d of reader %s (%"PRId64">=%d ratelimit ms + %d ms srvidhold!)",
						  reader->rlecmh[h].srvid, h + 1, MAXECMRATELIMIT, reader->label, gone,
			reader->rlecmh[h].ratelimittime, reader->rlecmh[h].srvidholdtime);
			reader->rlecmh[h].last.time = -1;
			reader->rlecmh[h].srvid 	 = -1;
			reader->rlecmh[h].kindecm 	 = 0;
		}
		if (reader->rlecmh[h].last.time == -1) { continue; }
		if (reader->rlecmh[h].ratelimitecm < maxecms) { maxecms = reader->rlecmh[h].ratelimitecm; }  // we found a more critical ratelimit srvid
		totalecms++;
	}

	cs_log_dbg(D_CLIENT, "ratelimiter found total of %d srvid for reader %s most critical is limited to %d requests", totalecms, reader->label, maxecms);

	if (reader->cooldown[0] && reader->cooldownstate != 1) { maxecms = MAXECMRATELIMIT; }  // dont apply ratelimits if cooldown isnt in use or not in effect

	for (h = 0; h < MAXECMRATELIMIT; h++) // check if srvid is already in a slot
	{
		if (reader->rlecmh[h].last.time == -1) { continue; }
		if (reader->rlecmh[h].srvid == er->srvid && reader->rlecmh[h].caid == rl.caid && reader->rlecmh[h].provid == rl.provid
			&& (!reader->rlecmh[h].chid || (reader->rlecmh[h].chid == rl.chid)))
		{
			int64_t gone = comp_timeb(&actualtime, &reader->rlecmh[h].last);
			cs_log_dbg(D_CLIENT, "ratelimiter found srvid %04X for %"PRId64" ms in slot %d/%d of reader %s", er->srvid,
						  gone, h + 1, MAXECMRATELIMIT, reader->label);

			// check ecmunique if enabled and ecmunique time is done
			if (reader_mode && reader->ecmunique)
			{
				gone = comp_timeb(&actualtime, &reader->rlecmh[h].last);
				if (gone < reader->ratelimittime)
				{
					if (memcmp(reader->rlecmh[h].ecmd5, er->ecmd5, CS_ECMSTORESIZE))
					{
						if (er->ecm[0]== reader->rlecmh[h].kindecm)
						{
							char ecmd5[17*3];
							cs_hexdump(0, reader->rlecmh[h].ecmd5, 16, ecmd5, sizeof(ecmd5));
							cs_log_dbg(D_CLIENT, "ratelimiter ecm %s in this slot for next %d ms!", ecmd5,
										  (int)(reader->rlecmh[h].ratelimittime - gone));

							struct ecm_request_t *erold=NULL;
							if (!cs_malloc(&erold, sizeof(struct ecm_request_t)))
								{ return -2; }
							memcpy(erold, er, sizeof(struct ecm_request_t)); // copy ecm all
							memcpy(erold->ecmd5, reader->rlecmh[h].ecmd5, CS_ECMSTORESIZE); // replace md5 hash
							struct ecm_request_t *ecm=NULL;
							ecm = check_cache(erold, erold->client); //CHECK IF FOUND ECM IN CACHE
							NULLFREE(erold);
							if (ecm)  //found in cache
								{ write_ecm_answer(reader, er, ecm->rc, ecm->rcEx, ecm->cw, &ecm->cwEx, NULL); }
							else
								{ write_ecm_answer(reader, er, E_NOTFOUND, E2_RATELIMIT, NULL, NULL, "Ratelimiter: no slots free!"); }

							NULLFREE(ecm);
							return -2;
						}
						continue;
					}
				}
				if ((er->ecm[0] == reader->rlecmh[h].kindecm)
						&& (gone <= (reader->ratelimittime + reader->srvidholdtime)))
				{

					cs_log_dbg(D_CLIENT, "ratelimiter srvid %04X ecm type %s, only allowing %s for next %d ms in slot %d/%d of reader %s -> skipping this slot!", reader->rlecmh[h].srvid, (reader->rlecmh[h].kindecm == 0x80 ? "even" : "odd"), (reader->rlecmh[h].kindecm == 0x80 ? "odd" : "even"),
								  (int)(reader->rlecmh[h].ratelimittime + reader->rlecmh[h].srvidholdtime - gone),
								h+1, maxecms, reader->label);
					continue;
				}
			}

			if (h > 0)
			{
				for (foundspace = 0; foundspace < h; foundspace++) // check for free lower slot
				{
					if (reader->rlecmh[foundspace].last.time == -1)
					{
						reader->rlecmh[foundspace] = reader->rlecmh[h]; // replace ecm request info
						reader->rlecmh[h].srvid = -1;
						reader->rlecmh[h].last.time = -1;
						if (foundspace < maxecms)
						{
							cs_log_dbg(D_CLIENT, "ratelimiter moved srvid %04X to slot %d/%d of reader %s", er->srvid, foundspace + 1, maxecms, reader->label);
							return foundspace; // moving to lower free slot!
						}
						else
						{
							cs_log_dbg(D_CLIENT, "ratelimiter removed srvid %04X from slot %d/%d of reader %s", er->srvid, foundspace + 1, maxecms, reader->label);
							reader->rlecmh[foundspace].last.time = -1; // free this slot since we are over ratelimit!
							return -1; // sorry, ratelimit!
						}
					}
				}
			}
			if (h < maxecms)    // found but cant move to lower position!
			{
				return h; // return position if within ratelimits!
			}
			else
			{
				reader->rlecmh[h].last.time = -1; // free this slot since we are over ratelimit!
				cs_log_dbg(D_CLIENT, "ratelimiter removed srvid %04X from slot %d/%d of reader %s", er->srvid, h + 1, maxecms, reader->label);
				return -1; // sorry, ratelimit!
			}
		}
	}

	// srvid not found in slots!

	if ((reader->cooldown[0] && reader->cooldownstate == 1) || !reader->cooldown[0])
	{
		; // do we use cooldown at all, are we in cooldown fase?

		// we are in cooldown or no cooldown configured!
		if (totalecms + 1 > maxecms || totalecms + 1 > rl.ratelimitecm)  // check if this channel fits in!
		{
			cs_log_dbg(D_CLIENT, "ratelimiter for reader %s has no free slots!", reader->label);
			return -1;
		}
	}
	else
	{
		maxecms = MAXECMRATELIMIT; // no limits right now!
	}

	for (h = 0; h < maxecms; h++)    // check for free slot
	{
		if (reader->rlecmh[h].last.time == -1)
		{
			if (reader_mode) { cs_log_dbg(D_CLIENT, "ratelimiter added srvid %04X to slot %d/%d of reader %s", er->srvid, h + 1, maxecms, reader->label); }
			return h; // free slot found -> assign it!
		}
		else {
			int64_t gone = comp_timeb(&actualtime, &reader->rlecmh[h].last);
			cs_log_dbg(D_CLIENT, "ratelimiter srvid %04X for %"PRId64" ms present in slot %d/%d of reader %s", reader->rlecmh[h].srvid, gone , h + 1,
					maxecms, reader->label);
		}  //occupied slots
	}

	foundspace = dvbapi_override_prio(reader, er, maxecms, &actualtime);
	if (foundspace > -1)
		return foundspace;

	return (-1); // no slot found
}

static void sort_ecmrl(struct s_reader *reader)
{
	int32_t i, j, loc;
	struct ecmrl tmp;

	for (i = 0; i < reader->ratelimitecm; i++)   // inspect all slots
	{
		if (reader->rlecmh[i].last.time == -1) { continue; }  // skip empty slots
		loc = i;
		tmp = reader->rlecmh[i]; // tmp is ecm in slot to evaluate

		for (j = i + 1; j < MAXECMRATELIMIT; j++)   // inspect all slots above the slot to be inspected
		{
			if (reader->rlecmh[j].last.time == -1) { continue; }  // skip empty slots
			int32_t gone = comp_timeb(&reader->rlecmh[i].last, &tmp.last);
			if (gone > 0)   // is higher slot holding a younger ecmrequest?
			{
				loc = j; // found a younger one
				tmp = reader->rlecmh[j]; // copy the ecm in younger slot
			}
		}

		if (loc != i)   // Did we find a younger ecmrequest?
		{
			reader->rlecmh[loc] = reader->rlecmh[i]; // place older request in slot of younger one we found
			reader->rlecmh[i] = tmp; // place younger request in slot of older request
		}
	}

	// release all slots above ratelimit ecm
	for (i = reader->ratelimitecm; i < MAXECMRATELIMIT; i++)
	{
		reader->rlecmh[i].last.time = -1;
		reader->rlecmh[i].srvid = -1;
	}

}

int32_t ecm_ratelimit_check(struct s_reader *reader, ECM_REQUEST *er, int32_t reader_mode)
// If reader_mode is 1, ECM_REQUEST need to be assigned to reader and slot.
// Else just report if a free slot is available.
{
	// No rate limit set
	if (!er) return OK;
	// sky(a)
	if ( er->ecm_bypass) return OK;
	if (!reader->ratelimitecm)
	{
		return OK;
	}

	int32_t foundspace = -1, h, maxslots = MAXECMRATELIMIT; //init slots to oscam global maximums
	struct ecmrl rl;
	struct timeb now;
	rl = get_ratelimit(er);

	if (rl.ratelimitecm > 0)
	{
		cs_log_dbg(D_CLIENT, "ratelimit found for CAID: %04X PROVID: %06X SRVID: %04X CHID: %04X maxecms: %d cycle: %d ms srvidhold: %d ms",
					  rl.caid, rl.provid, rl.srvid, rl.chid, rl.ratelimitecm, rl.ratelimittime, rl.srvidholdtime);
	}
	else // nothing found: apply general reader limits
	{
		rl.ratelimitecm = reader->ratelimitecm;
		rl.ratelimittime = reader->ratelimittime;
		rl.srvidholdtime = reader->srvidholdtime;
		rl.caid = er->caid;
		rl.provid = er->prid;
		rl.chid = er->chid;
		rl.srvid = er->srvid;
		cs_log_dbg(D_CLIENT, "ratelimiter apply readerdefault for CAID: %04X PROVID: %06X SRVID: %04X CHID: %04X maxecms: %d cycle: %d ms srvidhold: %d ms",
					  rl.caid, rl.provid, rl.srvid, rl.chid, rl.ratelimitecm, rl.ratelimittime, rl.srvidholdtime);
	}
	// Below this line: rate limit functionality.
	// No cooldown set
	if (!reader->cooldown[0])
	{
		cs_log_dbg(D_CLIENT, "ratelimiter find a slot for srvid %04X on reader %s", er->srvid, reader->label);
		foundspace = ecm_ratelimit_findspace(reader, er, rl, reader_mode);
		if (foundspace < 0)
		{
			if (reader_mode)
			{
				if (foundspace != -2)
				{
					cs_log_dbg(D_CLIENT, "ratelimiter no free slot for srvid %04X on reader %s -> dropping!", er->srvid, reader->label);
					write_ecm_answer(reader, er, E_NOTFOUND, E2_RATELIMIT, NULL, NULL, "Ratelimiter: no slots free!");
				}
			}

			return ERROR; // not even trowing an error... obvious reason ;)
		}
		else  //we are within ecmratelimits
		{
			if (reader_mode)
			{
				// Register new slot
			//	reader->rlecmh[foundspace].srvid=er->srvid; // register srvid
				reader->rlecmh[foundspace] = rl; // register this srvid ratelimit params
				cs_ftime(&reader->rlecmh[foundspace].last); // register request time
				memcpy(reader->rlecmh[foundspace].ecmd5, er->ecmd5, CS_ECMSTORESIZE);// register ecmhash
				reader->rlecmh[foundspace].kindecm = er->ecm[0]; // register kind of ecm
			}

			return OK;
		}
	}

	// Below this line: rate limit functionality with cooldown option.

	// Cooldown state cycle:
	// state = 0: Cooldown setup phase. No rate limit set.
	//	If number of ecm request exceed reader->ratelimitecm, cooldownstate goes to 2.
	// state = 2: Cooldown delay phase. No rate limit set.
	//	If number of ecm request still exceed reader->ratelimitecm at end of cooldown delay phase,
	//		cooldownstate goes to 1 (rate limit phase).
	//	Else return back to setup phase (state 0).
	// state = 1: Cooldown ratelimit phase. Rate limit set.
	//	If cooldowntime reader->cooldown[1] is elapsed, return to cooldown setup phase (state 0).

	cs_ftime(&now);
	int32_t	gone = comp_timeb(&now, &reader->cooldowntime);

	if (reader->cooldownstate == 1)    // Cooldown in ratelimit phase
	{
		if (gone <= reader->cooldown[1]*1000)  // check if cooldowntime is elapsed
			{ maxslots = reader->ratelimitecm; } // use user defined ratelimitecm
		else   // Cooldown time is elapsed
		{
			reader->cooldownstate = 0; // set cooldown setup phase
			reader->cooldowntime.time = -1; // reset cooldowntime
			maxslots = MAXECMRATELIMIT; //use oscam defined max slots
			cs_log("Reader: %s ratelimiter returning to setup phase cooling down period of %d seconds is done!",
				   reader->label, reader->cooldown[1]);
		}
	} // if cooldownstate == 1

	if (reader->cooldownstate == 2 && gone > reader->cooldown[0]*1000)
	{
		// Need to check if the otherslots are not exceeding the ratelimit at the moment that
		// cooldown[0] time was exceeded!
		// time_t actualtime = reader->cooldowntime + reader->cooldown[0];
		maxslots = 0; // maxslots is used as counter
		for (h = 0; h < MAXECMRATELIMIT; h++)
		{
			if (reader->rlecmh[h].last.time == -1) { continue; }  // skip empty slots
			// how many active slots are registered at end of cooldown delay period

			gone = comp_timeb(&now, &reader->rlecmh[h].last);
			if (gone <= reader->ratelimittime)
			{
				maxslots++;
				if (maxslots >= reader->ratelimitecm) { break; }  // Need to go cooling down phase
			}
		}

		if (maxslots < reader->ratelimitecm)
		{
			reader->cooldownstate = 0; // set cooldown setup phase
			reader->cooldowntime.time = -1; // reset cooldowntime
			maxslots = MAXECMRATELIMIT; // maxslots is maxslots again
			cs_log("Reader: %s ratelimiter returning to setup phase after %d seconds cooldowndelay!",
				   reader->label, reader->cooldown[0]);
		}
		else
		{
			reader->cooldownstate = 1; // Entering ratelimit for cooldown ratelimitseconds
			cs_ftime(&reader->cooldowntime); // set time to enforce ecmratelimit for defined cooldowntime
			maxslots = reader->ratelimitecm; // maxslots is maxslots again
			sort_ecmrl(reader); // keep youngest ecm requests in list + housekeeping
			cs_log("Reader: %s ratelimiter starting cooling down period of %d seconds!", reader->label, reader->cooldown[1]);
		}
	} // if cooldownstate == 2

	cs_log_dbg(D_CLIENT, "ratelimiter cooldownphase %d find a slot for srvid %04X on reader %s", reader->cooldownstate, er->srvid, reader->label);
	foundspace = ecm_ratelimit_findspace(reader, er, rl, reader_mode);

	if (foundspace < 0)
	{
		if (reader_mode)
		{
			if (foundspace != -2)
			{
				cs_log_dbg(D_CLIENT, "ratelimiter cooldownphase %d no free slot for srvid %04X on reader %s -> dropping!",
					reader->cooldownstate, er->srvid, reader->label);
				write_ecm_answer(reader, er, E_NOTFOUND, E2_RATELIMIT, NULL, NULL, "Ratelimiter: cooldown no slots free!");
			}
		}

		return ERROR; // not even trowing an error... obvious reason ;)
	}
	else  //we are within ecmratelimits
	{
		if (reader_mode)
		{
			// Register new slot
		//	reader->rlecmh[foundspace].srvid=er->srvid; // register srvid
			reader->rlecmh[foundspace] = rl; // register this srvid ratelimit params
			cs_ftime(&reader->rlecmh[foundspace].last); // register request time
			memcpy(reader->rlecmh[foundspace].ecmd5, er->ecmd5, CS_ECMSTORESIZE);// register ecmhash
			reader->rlecmh[foundspace].kindecm = er->ecm[0]; // register kind of ecm
		}
	}

	if (reader->cooldownstate == 0 && foundspace >= reader->ratelimitecm)
	{
		if (!reader_mode)    // No actual ecm request, just check
		{

			return OK;
		}
		cs_log("Reader: %s ratelimiter cooldown detected overrun ecmratelimit of %d during setup phase!",
			   reader->label, (foundspace - reader->ratelimitecm + 1));
		reader->cooldownstate = 2; // Entering cooldowndelay phase
		cs_ftime(&reader->cooldowntime); // Set cooldowntime to calculate delay
		cs_log_dbg(D_CLIENT, "ratelimiter cooldowndelaying %d seconds", reader->cooldown[0]);
	}

	// Cooldown state housekeeping is done. There is a slot available.
	if (reader_mode)
	{
		// Register new slot
	//	reader->rlecmh[foundspace].srvid = er->srvid; // register srvid
		reader->rlecmh[foundspace] = rl; // register this srvid ratelimit params
		cs_ftime(&reader->rlecmh[foundspace].last); // register request time
		memcpy(reader->rlecmh[foundspace].ecmd5, er->ecmd5, CS_ECMSTORESIZE);// register ecmhash
		reader->rlecmh[foundspace].kindecm = er->ecm[0]; // register kind of ecm
	}

	return OK;
}

const struct s_cardsystem *get_cardsystem_by_caid(uint16_t caid)
{
	int32_t i, j;
	for(i = 0; cardsystems[i]; i++)
	{
		const struct s_cardsystem *csystem = cardsystems[i];
		for(j = 0; csystem->caids[j]; j++)
		{
			uint16_t cs_caid = csystem->caids[j];
			if (!cs_caid)
				{ continue; }
			if (cs_caid == caid || cs_caid == caid >> 8)
				{ return csystem; }
		}
	}
	return NULL;
}

struct s_reader *get_reader_by_label(char *lbl)
{
	struct s_reader *rdr = 0;

	if (!lbl) return 0;
	LL_ITER itr = ll_iter_create(configured_readers);
	while ((rdr = ll_iter_next(&itr))) {
		if (streq(lbl, rdr->label))
			break;
	}
	return rdr;
}
// sky(n)
void chk_group_violation(struct s_reader *rdr)
{
	struct s_reader *rdr2 = 0;
	uint64_t	lgrps = 0;
	int i;

	if (!rdr) return;
	if ( rdr->grp) return;

	LL_ITER itr  = ll_iter_create(configured_readers);
	while ((rdr2 = ll_iter_next(&itr))) {
		if (rdr2->grp) lgrps |= rdr2->grp;
	}

	for (i=1; i<64; i++) {
		if ((lgrps>>i) & 1) continue;
		rdr->grp = 1<<i;
		mycs_trace(D_ADB, "myrdr:violation group=%llx", rdr->grp);
		break;
	}
}

struct s_reader *chk_sole_reader_protocols(char *protocols)
{
	if (!protocols) return NULL;
	struct protocol_map {
		char *name;
		int  typ;
	} sole_protocols[] = {
		{ "constcw",R_CONSTCW },
		{ "xcamd",	R_XCAMD},
		{ "morecam",R_MORECAM},
		{ "xcas",	R_XCAS},
		{ NULL, 		0 }
	}, *p;
	int32_t	i, typ = 0;

	// Parse protocols
	for (i = 0, p = &sole_protocols[0]; p->name; p = &sole_protocols[++i]) {
		if (streq(p->name, protocols)) {
			typ = p->typ;
			break;
		}
	}
	if (!typ) return NULL;
	struct s_reader *rdr = 0;
	LL_ITER itr = ll_iter_create(configured_readers);
	while ((rdr = ll_iter_next(&itr))) {
		if (rdr->typ == typ) return (rdr);
	}
	return NULL;
}

const char *reader_get_type_desc(struct s_reader *rdr, int32_t extended)
{
	const char *desc = "unknown";
	if (!rdr) return desc;
	if ( rdr->crdr && rdr->crdr->desc)
		{ return rdr->crdr->desc; }
	if (is_network_reader(rdr) || rdr->typ == R_SERIAL)
	{
		if (rdr->ph.desc)
			{ desc = rdr->ph.desc; }
	}
	if (rdr->typ == R_NEWCAMD && rdr->ncd_proto == NCD_524) desc = "newcamd524";
#if defined(MODULE_AVAMGCAMD)
	else if (rdr->typ == R_NEWCAMD && rdr->ncd_exprotocol == NCD_MGCAMD) desc = "mgcamd";
	else if (rdr->typ == R_NEWCAMD && rdr->ncd_exprotocol == NCD_AVATARCAMD) desc = "avatarcamd";
#endif
	else if (extended && rdr->typ == R_CCCAM && cccam_client_extended_mode(rdr->client))
	{
		desc = "cccam_ext";
	}
	return desc;
}

bool chk_reader_devices(struct s_reader *rdr)
{
	if (!strlen(rdr->device)) return 0;
	if (rdr->device[0] == 0)  return 0;
	if (rdr->r_pwd [0] == 0)  return 0;
	if (rdr->r_usr [0] == 0)  return 0;
	if (rdr->r_port    == 0)  return 0;
	return 1;
}

bool hexserialset(struct s_reader *rdr)
{
	int i;
	if (!rdr) return 0;
	for (i = 0; i < 8; i++) {
		if (rdr->hexserial[i]) return 1;
	}
	return 0;
}

void hexserial_to_newcamd(uchar *source, uchar *dest, uint16_t caid)
{
	if (caid_is_bulcrypt(caid))
	{
		dest[0] = 0x00;
		dest[1] = 0x00;
		memcpy(dest + 2, source, 4);
	}
	else if (caid_is_irdeto(caid) || caid_is_betacrypt(caid))
	{
		// only 4 Bytes Hexserial for newcamd clients (Hex Base + Hex Serial)
		// first 2 Byte always 00
		dest[0]=0x00; //serial only 4 bytes
		dest[1]=0x00; //serial only 4 bytes
		// 1 Byte Hex Base (see reader-irdeto.c how this is stored in "source")
		dest[2]=source[3];
		// 3 Bytes Hex Serial (see reader-irdeto.c how this is stored in "source")
		dest[3]=source[0];
		dest[4]=source[1];
		dest[5]=source[2];
	}
	else if (caid_is_viaccess(caid) || caid_is_cryptoworks(caid))
	{
		dest[0] = 0x00;
		memcpy(dest + 1, source, 5);
	}
	else
	{
		memcpy(dest, source, 6);
	}
}

void newcamd_to_hexserial(uchar *source, uchar *dest, uint16_t caid)
{
	if (caid_is_bulcrypt(caid))
	{
		memcpy(dest, source + 2, 4);
		dest[4] = 0x00;
		dest[5] = 0x00;
	}
	else if (caid_is_irdeto(caid) || caid_is_betacrypt(caid))
	{
		memcpy(dest, source+3, 3);
		dest[3] = source[2];
		dest[4] = 0;
		dest[5] = 0;
	}
	else if (caid_is_viaccess(caid) || caid_is_cryptoworks(caid))
	{
		memcpy(dest, source+1, 5);
		dest[5] = 0;
	}
	else
	{
		memcpy(dest, source, 6);
	}
}

/**
 * add or find one entitlement item to entitlements of reader
 * use add = 0 for find only, or add > 0 to find and add if not found
 **/
// sky(Add comments)
S_ENTITLEMENT *cs_add_entitlement(struct s_reader *rdr, uint16_t caid, uint32_t provid, uint64_t id, uint32_t class, time_t start, time_t end, uint8_t type, char *comments, uint8_t add)
{
	if(!rdr->ll_entitlements)
	{
		rdr->ll_entitlements = ll_create("ll_entitlements");
	}

	S_ENTITLEMENT *item = NULL;
	LL_ITER it;

	it = ll_iter_create(rdr->ll_entitlements);
	while((item = ll_iter_next(&it)) != NULL)
	{
		if (
			(caid && item->caid != caid) ||
			(provid && item->provid != provid) ||
			(id && item->id != id) ||
			(class && item->class != class) ||
			(start && item->start != start) ||
			(end && item->end != end) ||
			(type && item->type != type))
		{
			continue; // no match, try next!
		}
		break; // match found!
	}

	if(add && item == NULL)
	{
	   if (cs_malloc(&item, sizeof(S_ENTITLEMENT)))
	   {
		   // fill item
		   item->caid 	 = caid;
		   item->provid = provid;
		   item->id 	 = id;
		   item->class  = class;
		   item->start  = start;
		   item->end 	 = end;
		   item->type 	 = type;
		   item->comments[0] = 0;
		   if (comments) strcpy(item->comments, comments);
		   // add item
		   ll_append(rdr->ll_entitlements, item);
   	//	cs_log_dbg(D_TRACE, "entitlement: Add caid %4X id %4X %s - %s ", item->caid, item->id, item->start, item->end);
	   }
		else
		{
			cs_log("ERROR: Can't allocate entitlement to reader!");

		}
	}

	return item;
}
//
//
// sky(oscam.smartcard)
// sky(sim)
extern int __system_property_set(const char *key, char *value);
void cs_clean_cardinformation(void)
{
	char viewfile[256] = { 0 };

	__system_property_set("service.sci.state", "");
	get_information_filename(viewfile, sizeof(viewfile), cs_SMCINFORMATION);
	if (file_exists(viewfile))	{
		cs_log("removing scinformation %s", viewfile);
		if (unlink(viewfile) < 0) {
			cs_log("Error removing scinformation file %s (errno=%d %s)!", viewfile, errno, strerror(errno));
		}
	}
}

int cs_save_cardinformation(struct s_reader *rdr)
{
	char viewfile[256] = { 0 };
	FILE *fp;
	char tbuffer1[64], tbuffer2[64], buf[256] = { 0 }, tmpbuf[256] = { 0 }, valid_to[32] = { 0 };
	char tmp[512];
	uint16_t casysid;
	int32_t  i;

	get_information_filename(viewfile, sizeof(viewfile), cs_SMCINFORMATION);
	fp = fopen(viewfile, "w");
	if (!fp) {
		cs_log("ERROR: Cannot create file \"%s\" (errno=%d %s)", viewfile, errno, strerror(errno));
		return 0;
	}
	setvbuf(fp, NULL, _IOFBF, 4 * 1024);
	fprintf(fp, "# smc information generated automatically by sky.\n");
	fprintf(fp, "\n");
	if (!rdr) {
		fprintf(fp, "%s\n", "; Reader do not exist or it is not started.");
		fclose (fp);
		return 0;
	}

	casysid = rdr->caid & 0xff00;
	fprintf(fp, "[INFORMATIONS]\n");
	if (!rdr->enable) {
		__system_property_set("service.sci.state", "disable");
		fprintf(fp, "  status     : disabled\n");
		fclose (fp);
		return 0;
	}
	if (rdr->card_status == NO_CARD)
	{
		__system_property_set("service.sci.state", "nocard");
		fprintf(fp, "  status     : no card\n");
		fclose(fp);
		return 0;
	}

	fprintf(fp, "  Reader     : %s\n",   rdr->label);
	fprintf(fp, "  Cardsystem : %s\n",  (rdr->csystem && rdr->csystem->desc) ? rdr->csystem->desc : "unknown");
	fprintf(fp, "  Casysid    : %04X\n", rdr->caid);
	fprintf(fp, "  Serial     : %s\n",   rdr->ascserial);
	if (casysid == 0x0600)
	{
		fprintf(fp, "  Acs        : %04X\n",   rdr->acs);
		fprintf(fp, "  Nationality: %c%c%c\n", rdr->country_code[0],rdr->country_code[1],rdr->country_code[2]);
	}
	else
	if (casysid == 0x0B00)
	{
		fprintf(fp, "  Ver.       : %02X\n", rdr->cardver);
		fprintf(fp, "  Maturity   : %d\n",   rdr->maturity);
	}
	else
	if (casysid == 0x0500)
	{
		fprintf(fp, "  Maturity   : %d\n",   rdr->maturity);
	}
	else
	if (casysid == 0x0D00)
	{
		fprintf(fp, "  Maturity   : %d\n",   rdr->maturity);
	}
//	fprintf(fp, "  HexSerial  : %s\n", cs_hexdump(1, rdr->hexserial, 8, tbuffer2, sizeof(tbuffer2)));
	fprintf(fp, "  ATR        : %s\n", (rdr->card_atr_length) ? cs_hexdump(1, rdr->card_atr, rdr->card_atr_length, buf, sizeof(buf)) : "?");
	fprintf(fp, "\n");
	//
	//
	//
	if (casysid == 0x0100 || casysid == 0x0500 || casysid == 0x0b00 || casysid == 0x0d00)
	{
		fprintf(fp, "  Pincode    : %s\n", rdr->pincode);
	}
	if (casysid == 0x0900)
	{
		fprintf(fp, "  Boxid      : %08X\n", rdr->boxid);
	}
	if (casysid == 0x0600 || casysid == 0x1800)
	{
		fprintf(fp, "  Boxkey     : %s\n", cs_hexdump(0, rdr->boxkey, sizeof(rdr->boxkey), tmp, sizeof(tmp)));
	}
	if (casysid == 0x0600 || casysid == 0x1800 || casysid == 0xB00)
	{
		int32_t len = check_filled(rdr->rsa_mod, 120);
		if (len > 0) {
			len = (len > 64) ? 120 : 64;
			fprintf(fp, "  Rsakey     : %s\n", cs_hexdump(0, rdr->rsa_mod, len, tmp, sizeof(tmp)));
		} else
			fprintf(fp, "  Rsakey     : none\n");
	}
	fprintf(fp, "  Autoroll   : %s\n", (rdr->audisabled) ? "disable" : "enable" );
	//
	//
	//
	//
	if (rdr->card_valid_to) {
		struct tm vto_t;
		localtime_r(&rdr->card_valid_to, &vto_t);
		strftime(valid_to, sizeof(valid_to) - 1, "%Y-%m-%d", &vto_t);
		fprintf(fp, "  ValidTo    : %s\n", valid_to);
	}
	if (rdr->card_status == CARD_FAILURE)
	{
		if (rdr->card_atr_length) {
			__system_property_set("service.sci.state", "atr");
		}
		else {
		   __system_property_set("service.sci.state", "ng");
		}
		fprintf(fp, "  status     : failure\n");
		fprintf(fp, "\n");
		fclose (fp);
		return 0;
	}
	__system_property_set("service.sci.state", "ok");
	if (casysid == 0x1800) {
		fprintf(fp, "  status     : %s\n", rdr->nagra_negotiate ? "success" : "unsettled");
	}
	else {
		fprintf(fp, "  status     : success\n");
	}
	fprintf(fp, "\n");

	fprintf(fp, "[ENTITLEMENTS]\n");
	if (rdr->ll_entitlements)
	{
		S_ENTITLEMENT *item;
		LL_ITER itr = ll_iter_create(rdr->ll_entitlements);
		time_t now = (time(NULL) / 84600) * 84600;

		while ((item = ll_iter_next(&itr))) {
			struct tm start_t, end_t;

			localtime_r(&item->start, &start_t);
			localtime_r(&item->end  , &end_t);
			strftime(tbuffer1, sizeof(tbuffer1) - 1, "%d-%m-%Y", &start_t);
			strftime(tbuffer2, sizeof(tbuffer2) - 1, "%d-%m-%Y", &end_t);

			snprintf(tmpbuf, sizeof(tmpbuf) - 1, "%s:: %-9s: provid.%06X (%s ~ %s) ",
					item->end > now ? "Active " : "Expired",
					entitlement_type[item->type],
					item->provid,
					tbuffer1,
					tbuffer2);
			if (item->comments[0])
			{
				strcat(tmpbuf, item->comments);
			}
			else
			{
				char *entresname = get_tiername(item->id & 0xFFFF, item->caid, buf);
				if (!entresname[0]) entresname = get_provider_existnace(item->caid, item->provid, buf, sizeof(buf));
				if ( entresname[0]) {
					strcat(tmpbuf, "Name: ");
					strcat(tmpbuf, entresname);
				}
			}
			fprintf(fp, "  %s\n", tmpbuf);
		}
	}
	else
	{
		fprintf(fp, "; No entitlements.\n");
	}

	fprintf(fp, "\n");
	fprintf(fp, "[PROVIDERS]\n");
	if (rdr->nprov)
	{
		for (i= 0; i<rdr->nprov; i++)
		{
			fprintf(fp, "  %2d. %04X:%06X\n", i, rdr->caid, b2i(4, rdr->prid[i]));
		}
	}
	else
	{
		fprintf(fp, "; No providers.\n");
	}
	fprintf(fp, "\n");
	fclose(fp);
	return 1;
}

/**
 * clears entitlements of reader.
 **/
void cs_clear_entitlement(struct s_reader *rdr)
{
	if (!rdr) return;
	if (!rdr->ll_entitlements) return;

	ll_clear_data(rdr->ll_entitlements);
}


void casc_check_dcw(struct s_reader *reader, int32_t idx, int32_t cwfound, uchar *cw, CWEXTENTION *cwEx)
{
	struct s_client *cl = reader->client;
	ECM_REQUEST *ere;
	ECM_REQUEST *chkere;
	int32_t i, pending = 0;
	time_t  t = time(NULL);

	if (!check_client(cl)) return;
	chkere = &(cl->ecmtask[idx]);
	MYREADER_TRACE("myrdr:casc_check_dcw=%d,%d, %04x{%d,%s}\n", idx, cwfound, chkere->caid, cfg.max_pending, reader->label);
	// sky(a)
	if (chkere->ecm_bypass)
	{
		if (!cwfound) return;
		mycs_trace(D_ADB, "myrdr:casc_check_dcw.ecm_skip{%d,%d, %04X:%06X.%5d}",
					idx,
					chkere->rc,
					chkere->caid, chkere->prid, chkere->srvid);
		cs_ftime(&chkere->tps);
		write_ecm_answer(reader, chkere, E_FOUND, 0, cw, NULL, NULL);
		cl->last_srvid = chkere->srvid;
		cl->last_caid  = chkere->caid;
	 	cl->last 		= t;
		cl->pending 	= pending;
		reader->last_g = t;
		chkere->rc  	= E_FOUND;
		return;
	}

	for (i = 0; i < cfg.max_pending; i++)
	{
		ere = &(cl->ecmtask[i]);

		if ((ere->rc >= E_NOCARD) && ere->caid == chkere->caid && (!memcmp(ere->ecmd5, chkere->ecmd5, CS_ECMSTORESIZE)))
		{
			if (cwfound==2)  //E_INVALID from camd35 CMD08
			{
				write_ecm_answer(reader, ere, E_INVALID, 0, cw, cwEx, NULL);
			}
			else if (cwfound) {
				write_ecm_answer(reader, ere, E_FOUND,   0, cw, cwEx, NULL);
			}
			else {
				write_ecm_answer(reader, ere, E_NOTFOUND,0, NULL, NULL, NULL);
			}
			ere->idx= 0;
			ere->rc = E_FOUND;
		}

		if ((ere->rc >= E_NOCARD) && (t-(uint32_t)ere->tps.time > ((cfg.ctimeout + 500) / 1000) + 1)) { // drop timeouts
			ere->rc = E_FOUND;
		}

		if (ere->rc >= E_NOCARD) pending++;
	}
	cl->pending = pending;
}

int32_t hostResolve(struct s_reader *rdr)
{
   struct s_client *cl = rdr->client;

   if (!cl) return 0;

   IN_ADDR_T last_ip;
   IP_ASSIGN(last_ip, cl->ip);
   cs_resolve(rdr->device, &cl->ip, &cl->udp_sa, &cl->udp_sa_len);
   IP_ASSIGN(SIN_GET_ADDR(cl->udp_sa), cl->ip);

   if (!IP_EQUAL(cl->ip, last_ip))
   {
   	if (IS_SERVER_EMBEDDED(rdr)) {
   	}
   	else {
			if (cfg.logsvrsecrete) {
		  		rdr_log_tildes(rdr, "resolve ip {%s}", cs_inet_ntoa(cl->ip));
			}
			else {
		  		rdr_log(rdr, "resolved ip %s", cs_inet_ntoa(cl->ip));
			}
	   }
   }

   return IP_ISSET(cl->ip);
}

void clear_block_delay(struct s_reader *rdr)
{
	if (!rdr) return;
   rdr->tcp_block_delay = 0;
   cs_ftime(&rdr->tcp_block_connect_till);
}

void block_connect(struct s_reader *rdr)
{
	if (!rdr->tcp_block_delay)
		{ rdr->tcp_block_delay = 100; } // starting blocking time, 100ms
	cs_ftime(&rdr->tcp_block_connect_till);
	add_ms_to_timeb(&rdr->tcp_block_connect_till, rdr->tcp_block_delay);
	rdr->tcp_block_delay *= 4; // increment timeouts
	if (rdr->tcp_block_delay >= rdr->tcp_reconnect_delay)
		{ rdr->tcp_block_delay = rdr->tcp_reconnect_delay; }
	rdr_log_dbg(rdr, D_TRACE, "tcp connect blocking delay set to %d", rdr->tcp_block_delay);
}

int32_t is_connect_blocked(struct s_reader *rdr)
{
	struct timeb cur_time;
	cs_ftime(&cur_time);
	int32_t diff = comp_timeb(&cur_time, &rdr->tcp_block_connect_till);
	// sky(a)
	// wrong time,86400(1970-01-01~)
	if (rdr->tcp_block_connect_till.time < 40000000) {
		rdr_log_dbg(rdr, D_TRACE, "connection time wrong(%d)", (int)rdr->tcp_block_connect_till.time);
		return 0;
	}
	int32_t blocked = rdr->tcp_block_delay && diff < 0;
	if (blocked) {
		rdr_log_dbg(rdr, D_TRACE, "connection blocked, retrying in %d ms", -diff);
	}
	return blocked;
}

// sky(Add)
int32_t
network_chk_intefaces(uint32_t *ip, uint8_t *mac)
{
	struct SOCKADDR *s_in;
	struct ifreq ifr;
	int sockfd;

	// no mac address specified so use mac of eth0 on local box
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 1) return 0;

	memset(&ifr, 0, sizeof(struct ifreq));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth0");
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
	{
		mycs_trace(D_ADB, "camd:!!! siocgifhwaddr(eth0) fail");
		close(sockfd);
	 	return 0;
	}

	if (mac) memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
	if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
	{
		mycs_trace(D_ADB, "camd:!!! siocgifaddr(eth0) fail");
		// for wifi
		#if 0
				memset(&ifr, 0, sizeof(struct ifreq));
				snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "wlan0");
				if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
				{
					mycs_trace(D_ADB, "camd:!!! siocgifhwaddr(wlan0) fail");
					close(sockfd);
					return;
				}
				memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
				if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
				{
					mycs_trace(D_ADB, "camd:!!! siocgifaddr(wlan0) fail");
					close(sockfd);
					return;
				}
		#endif
	}
	if (ip) {
		s_in = (struct sockaddr_in *)(&ifr.ifr_addr);
		*ip = s_in->sin_addr.s_addr;
	}
	close(sockfd);
	if (mac) {
		MYREADER_TRACE("camd:MAC:%x.%x.%x.%x.%x.%x\n",
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
	}
	return 1;
}

int32_t
network_tcp_socket_open(struct s_reader *rdr, char *devices, int32_t rport)
{
	if (!rdr) return -1;
	struct SOCKADDR loc_sa;
	struct SOCKADDR tcp_sa;
	IN_ADDR_T ip;
	IN_ADDR_T last_ip;
	socklen_t sa_len;
	int32_t	 sockfd;

	MYREADER_TRACE("mysocket:connect to %s,%d\n", devices, rport);

	memset((char *)&tcp_sa, 0, sizeof(tcp_sa));
	IP_ASSIGN(last_ip, ip);

   cs_resolve(devices, &ip, &tcp_sa, &sa_len);
   IP_ASSIGN(SIN_GET_ADDR(tcp_sa), ip);
 	if (!IP_ISSET(ip)) {
		MYREADER_TRACE("mysocket:hostResolve(%s) failure\n", devices);
		return -1;
	}
  if (!IP_EQUAL(ip, last_ip)) {
		clear_block_delay(rdr);
  		MYREADER_TRACE("mysocket:resolved ip %s\n", cs_inet_ntoa(ip));
   }
	if (is_connect_blocked(rdr)) { // inside of blocking delay, do not connect!
		return -1;
	}
	if (rport <= 0) {
		MYREADER_TRACE("mysocket:invalid port %d for server %s\n", rport, devices);
		return -1;
	}

	int s_domain = PF_INET;
	int s_family = AF_INET;
#ifdef IPV6SUPPORT
	if (!IN6_IS_ADDR_V4MAPPED(&ip) && !IN6_IS_ADDR_V4COMPAT(&ip))
	{
		s_domain  = PF_INET6;
		s_family  = AF_INET6;
	}
#endif
	int s_type   = SOCK_STREAM;
	int s_proto  = IPPROTO_TCP;

	if ((sockfd = socket(s_domain, s_type, s_proto)) < 0)
	{
		MYREADER_TRACE("mysocket:Socket creation failed (errno=%d,%s)\n", errno, strerror(errno));
		block_connect(rdr);
		return -1;
	}

	set_socket_priority(sockfd, cfg.netprio);

	int32_t keep_alive = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keep_alive, sizeof(keep_alive));

	int32_t flag = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(flag)) < 0)
	{
		MYREADER_TRACE("mysocket:setsockopt failed (errno=%d,%s)\n", errno, strerror(errno));
		block_connect(rdr);
		sockfd = 0;
		return -1;
	}

#ifdef SO_REUSEPORT
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&flag, sizeof(flag));
#endif

	memset((char *)&loc_sa,0,sizeof(loc_sa));
	SIN_GET_FAMILY(loc_sa) = s_family;
	SIN_GET_ADDR(loc_sa) = ADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&loc_sa, sizeof (loc_sa)) < 0) {
		MYREADER_TRACE("mysocket:bind failed (errno=%d,%s)\n", errno, strerror(errno));
		block_connect(rdr);
		close(sockfd);
		sockfd = 0;
		return -1;
	}

#ifdef IPV6SUPPORT
	if (IN6_IS_ADDR_V4MAPPED(&ip) || IN6_IS_ADDR_V4COMPAT(&ip)) {
		((struct sockaddr_in  *)(&tcp_sa))->sin_family  = AF_INET;
		((struct sockaddr_in  *)(&tcp_sa))->sin_port    = htons((uint16_t)rport);
	}
	else {
		((struct sockaddr_in6 *)(&tcp_sa))->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)(&tcp_sa))->sin6_port   = htons((uint16_t)rport);
	}
#else
	tcp_sa.sin_family = AF_INET;
	tcp_sa.sin_port   = htons((uint16_t)rport);
#endif

	MYREADER_TRACE("mysocket:socket open for fd=%d\n", sockfd);
	set_nonblock(sockfd, true);

	int32_t res = connect(sockfd, (struct sockaddr *)&tcp_sa, sa_len);
	if (res == -1)
	{
		int32_t r = -1;
		int32_t waitMs = 8000; // sky(cccam:tv2020),3000
		if (errno == EINPROGRESS || errno == EALREADY)
		{
			MYREADER_TRACE("mysocket:connect(%d) waiting...\n", errno);
			struct pollfd pfd;
			pfd.fd = sockfd;
			pfd.events = POLLOUT;
			int32_t rc = poll(&pfd, 1, waitMs);
			if (rc > 0)
			{
				uint32_t l = sizeof(r);
				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &r, (socklen_t *)&l) != 0)
					{ r = -1; }
				else
					{ errno = r; }
			}
			else
			{
				errno = ETIMEDOUT;
			}
		}
		if (r != 0) {
			MYREADER_TRACE("mysocket:connect(%d) failure.%s\n", errno, strerror(errno));
			block_connect(rdr);
			close(sockfd);
			sockfd = 0;
			return -1;
		}
	}

	set_nonblock(sockfd, false); //restore blocking mode
	setTCPTimeouts(sockfd);
	clear_block_delay(rdr);
	MYREADER_TRACE("mysocket:connect successful fd=%d\n", sockfd);
	return (sockfd);
}

void
network_tcp_socket_close(struct s_reader *reader, int32_t sockfd)
{
	if (!reader) return;
	MYREADER_TRACE("mysocket:disconnect to %d\n", sockfd);
	if (sockfd)
	{
		shutdown(sockfd, SHUT_RDWR);	// Send a FIN here
		close(sockfd);
	}
}


int32_t network_tcp_connection_open(struct s_reader *rdr)
{
	if (!rdr) return -1;
	if (!rdr->client) return -1;
	struct s_client *client = rdr->client;
	struct SOCKADDR loc_sa;

	MYREADER_TRACE("myrdr:connect to %s,%d(%s)\n", rdr->device, rdr->r_port, rdr->label);
	if (cfg.logsvrsecrete) {
		rdr_log_tildes(rdr, "connect to {%s},{%d}\n", T_READER_DEVICES(rdr), T_READER_RPORTS(rdr));
	}
	else {
		rdr_log(rdr, "connect to %s,%d\n", T_READER_DEVICES(rdr), T_READER_RPORTS(rdr));
	}
	memset((char *)&client->udp_sa, 0, sizeof(client->udp_sa));

	IN_ADDR_T last_ip;
	IP_ASSIGN(last_ip, client->ip);
	if (!hostResolve(rdr)) {
		MYREADER_TRACE("hostResolve(%s) failure\n", client->reader->device);
		return -1;
	}
	if (!IP_EQUAL(last_ip, client->ip)) { // clean blocking delay on ip change:
		clear_block_delay(rdr);
	}
	if (is_connect_blocked(rdr)) { // inside of blocking delay, do not connect!
		return -1;
	}

	if (client->reader->r_port <= 0) {
		if (cfg.logsvrsecrete) {
			rdr_log_tildes(client->reader, "invalid port {%d} for server {%s}", T_READER_RPORTS(client->reader), T_READER_DEVICES(client->reader));
		}
		else {
			rdr_log(client->reader, "invalid port %d for server %s", T_READER_RPORTS(client->reader), T_READER_DEVICES(client->reader));
		}
		return -1;
	}

	int bindCounter = 1;
	client->is_udp = (rdr->typ==R_CAMD35) ? 1 : 0;

	if (client->udp_fd) {
		rdr_log(rdr, "WARNING: client->udp_fd was not 0");
	}
	int s_domain = PF_INET;
	int s_family = AF_INET;
#ifdef IPV6SUPPORT
	if (!IN6_IS_ADDR_V4MAPPED(&rdr->client->ip) && !IN6_IS_ADDR_V4COMPAT(&rdr->client->ip))
	{
		s_domain  = PF_INET6;
		s_family  = AF_INET6;
	}
#endif
	int s_type   = client->is_udp ? SOCK_DGRAM  : SOCK_STREAM;
	int s_proto  = client->is_udp ? IPPROTO_UDP : IPPROTO_TCP;

	if ((client->udp_fd = socket(s_domain, s_type, s_proto)) < 0)
	{
		rdr_log(rdr, "Socket creation failed (errno=%d,%s)", errno, strerror(errno));
		client->udp_fd = 0;
		block_connect(rdr);
		return -1;
	}

	set_socket_priority(client->udp_fd, cfg.netprio);

	int32_t keep_alive = 1;
	setsockopt(client->udp_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keep_alive, sizeof(keep_alive));

	int32_t flag = 1;
	setsockopt(client->udp_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));

	if (setsockopt(client->udp_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(flag)) < 0)
	{
		rdr_log(rdr, "setsockopt failed (errno=%d,%s)", errno, strerror(errno));
		client->udp_fd = 0;
		block_connect(rdr);
		return -1;
	}

	set_so_reuseport(client->udp_fd);

	memset((char *)&loc_sa,0,sizeof(loc_sa));
	SIN_GET_FAMILY(loc_sa) = s_family;
	if (IP_ISSET(cfg.srvip))
		{ IP_ASSIGN(SIN_GET_ADDR(loc_sa), cfg.srvip); }
	else
		{ SIN_GET_ADDR(loc_sa) = ADDR_ANY; }

	if (client->reader->l_port)
	{
		do
		{
			SIN_GET_PORT(loc_sa) = htons(client->reader->l_port);
			if (bind(client->udp_fd, (struct sockaddr *)&loc_sa, sizeof (loc_sa)) >= 0) break;
			client->reader->l_port++;
		} while (--bindCounter);
		if (bindCounter==0) {
			MYREADER_TRACE("myrdr:bind failed (errno=%d,%s)", errno, strerror(errno));
			rdr_log(rdr, "bind failed (errno=%d,%s)", errno, strerror(errno));
			close(client->udp_fd);
			client->udp_fd = 0;
			block_connect(rdr);
			return -1;
		}
	}
	else
	{
	//	if (bind(client->udp_fd, (struct sockaddr *)&loc_sa, sizeof (loc_sa)) < 0) {
		if (client->is_udp && bind(client->udp_fd, (struct sockaddr *)&loc_sa, sizeof(loc_sa)) < 0)
		{
			rdr_log(rdr, "bind failed (errno=%d,%s)", errno, strerror(errno));
			close(client->udp_fd);
			client->udp_fd = 0;
			block_connect(rdr);
			return -1;
		}
	}

#ifdef IPV6SUPPORT
	if (IN6_IS_ADDR_V4MAPPED(&rdr->client->ip) || IN6_IS_ADDR_V4COMPAT(&rdr->client->ip)) {
		((struct sockaddr_in  *)(&client->udp_sa))->sin_family  = AF_INET;
		((struct sockaddr_in  *)(&client->udp_sa))->sin_port    = htons((uint16_t)client->reader->r_port);
	}
	else {
		((struct sockaddr_in6 *)(&client->udp_sa))->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)(&client->udp_sa))->sin6_port   = htons((uint16_t)client->reader->r_port);
	}
#else
	client->udp_sa.sin_family = AF_INET;
	client->udp_sa.sin_port   = htons((uint16_t)client->reader->r_port);
#endif

	rdr_log_dbg(rdr, D_TRACE, "socket open fd=%d", client->udp_fd);

	if (client->is_udp) {
		rdr->tcp_connected = 1;
		return (client->udp_fd);
	}

	set_nonblock(client->udp_fd, true);

	int32_t res = connect(client->udp_fd, (struct sockaddr *)&client->udp_sa, client->udp_sa_len);
	if (res == -1)
	{
		int32_t r = -1;
		int32_t waitMs = 8000; // sky(cccam:tv2020),3000
		if (errno == EINPROGRESS || errno == EALREADY)
		{
			rdr_log(rdr, "connect(%d) waiting...", errno);
			struct pollfd pfd;
			pfd.fd = client->udp_fd;
			pfd.events = POLLOUT;
			int32_t rc = poll(&pfd, 1, waitMs);
			if (rc > 0)
			{
				uint32_t l = sizeof(r);
				if (getsockopt(client->udp_fd, SOL_SOCKET, SO_ERROR, &r, (socklen_t *)&l) != 0)
					{ r = -1; }
				else
					{ errno = r; }
			}
			else
			{
				errno = ETIMEDOUT;
			}
		}
		if (r != 0) {
			rdr_log(rdr, "connect(%d) failure.%s", errno, strerror(errno));
			block_connect(rdr); // connect has failed. Block connect for a while
			close(client->udp_fd);
			client->udp_fd = 0;
			return -1;
		}
	}

	set_nonblock(client->udp_fd, false); //restore blocking mode

	setTCPTimeouts(client->udp_fd);
	clear_block_delay(rdr);
	client->pfd  = client->udp_fd;
	client->last = client->login = time((time_t *)0);
	client->last_caid  = NO_CAID_VALUE;
	client->last_srvid = NO_SRVID_VALUE;
	rdr->tcp_connected = 1;
	rdr_log_dbg(rdr, D_TRACE, "connect successful fd=%d", client->udp_fd);
	return (client->udp_fd);
}

void network_tcp_connection_close(struct s_reader *reader, char *reason)
{
	if (!reader) {
		//only proxy reader should call this, client connections are closed on thread cleanup
		cs_log("WARNING: invalid client");
		cs_disconnect_client(cur_client());
		return;
	}

	struct s_client *cl = reader->client;
	if (!cl) return;

	int32_t fd = cl->udp_fd;
	int32_t i;

	if (fd)
	{
		rdr_log(reader, "disconnected(%s)", reason ? reason : "undef");
//		MYREADER_TRACE("myrdr:disconnected %s(%s):%s\n", reader->device, reader->label, (reason) ? reason : "undef");

		#if 1
		// sky(!)
		// for remove TIME_WAIT status
		// Linger Options
		//	struct linger lng;
		//	lng.l_onoff  = 1;
		//	lng.l_linger = 0;
		//	setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&lng, sizeof(lng));
			shutdown(fd, SHUT_RDWR);	// Send a FIN here
		#endif
		close(fd);

		cl->udp_fd = 0;
		cl->pfd = 0;
	}

	reader->tcp_connected = 0;
	reader->card_status   = UNKNOWN;
	cl->logout = time((time_t *)0);

	if (cl->ecmtask)
	{
		for (i = 0; i < cfg.max_pending; i++) {
			cl->ecmtask[i].idx = 0;
			cl->ecmtask[i].rc  = E_FOUND;
		}
	}
	// newcamd message ids are stored as a reference in ecmtask[].idx
 	// so we need to reset them aswell
	if (reader->typ == R_NEWCAMD)
		cl->ncd_msgid = 0;
}

int32_t casc_process_ecm(struct s_reader *reader, ECM_REQUEST *er)
{
	struct s_client *cl = reader->client;
	int32_t rc, n, i, sending, pending=0;
	// sky(n)
	int32_t nfirst = 0, niniz = -1;
	time_t  t;//, tls;

	mycs_trace(D_ADB, "myrdr:casc_process_ecm{%s}", reader->label);
	if (!cl || !cl->ecmtask) {
		rdr_log(reader, "WARNING: ecmtask not a available");
		return -1;
	}
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (IS_ICS_READERS(reader)) {
		if (er->ecm_bypass || er->ecm_useless) return -1;
		niniz = 0; nfirst = 1;
	}
#endif

	uchar buf[512];

	t = time((time_t *)0);
	ECM_REQUEST *ere;
	for (i = nfirst; i < cfg.max_pending; i++)
	{
		ere = &cl->ecmtask[i];
		if (((ere->rc >= E_NOCARD) && (t-(uint32_t)ere->tps.time > ((cfg.ctimeout + 500) / 1000) + 1))) { // drop timeouts
			ere->rc = E_FOUND;
		}
	}
	for (n = niniz, i = nfirst, sending = 1; i < cfg.max_pending; i++)
	{
		ere = &cl->ecmtask[i];
		if (n<nfirst && (ere->rc < E_NOCARD))	// free slot found
			n = i;

		// ecm already pending
		// ... this level at least
		if ((ere->rc >= E_NOCARD) && er->caid == ere->caid && (!memcmp(er->ecmd5, ere->ecmd5, CS_ECMSTORESIZE)))
			sending = 0;

		if (ere->rc >= E_NOCARD) pending++;
	}
	cl->pending = pending;

	if (n<0) {
		rdr_log(reader, "WARNING: reader ecm pending table overflow !!");
		return (-2);
	}
	memcpy(&cl->ecmtask[n], er, sizeof(ECM_REQUEST));

	cl->ecmtask[n].parent = er;
	cl->ecmtask[n].matching_rdr  = NULL; // This avoids double free of matching_rdr!
#ifdef CS_CACHEEX
	cl->ecmtask[n].csp_lastnodes = NULL; // This avoids double free of csp_lastnodes!
#endif

// sky(n)
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (IS_ICS_READERS(reader))
	{
		// sky(uk)
		if (XCAMDIKS_Isv2()) {
			cl->ch_ecmsequece++;
	      if (cl->ch_ecmsequece < ERE_CS_FIRST) cl->ch_ecmsequece = ERE_CS_FIRST;
	      if (cl->ch_ecmsequece > ERE_CS_FINAL) cl->ch_ecmsequece = ERE_CS_FIRST;
		   mycs_trace(D_ADB, "myrdr:ecm.sequence{%d}{%04X}{s.%d}",
				   n, cl->ecmtask[n].caid, cl->ch_ecmsequece);
			cl->ecmtask[n].idx = cl->ch_ecmsequece;
		}
	}
	else
#endif
	if (reader->typ == R_NEWCAMD)
		cl->ecmtask[n].idx = (cl->ncd_msgid==0) ? 2 : cl->ncd_msgid+1;
	else {
		if (!cl->idx) cl->idx = 1;
		cl->ecmtask[n].idx = cl->idx++;
	}

	cl->ecmtask[n].rc = E_NOCARD;
	cs_log_dbg(D_TRACE, "---- ecm_task %d, idx %d, sending=%d", n, cl->ecmtask[n].idx, sending);
	cs_log_dump_dbg(D_TRACE, er->ecm, er->ecmlen, "casc ecm (%s):", (reader) ? reader->label : "n/a");
	rc = 0;
	if (sending)
	{
		rc = reader->ph.c_send_ecm(cl, &cl->ecmtask[n], buf);
		MYREADER_TRACE("myrdr:c_send_ecm{%s.%d}\n", reader->label, rc);
		if (rc)
			casc_check_dcw(reader, n, 0, cl->ecmtask[n].cw, &cl->ecmtask[n].cwEx);  // simulate "not found"
		else
			cl->last_idx = cl->ecmtask[n].idx;
		reader->last_s = t;   // used for inactive_timeout and reconnect_timeout in TCP reader
	}

	if (cl->idx > 0x1ffe) cl->idx = 1;
	return (rc);
}


void reader_get_ecm(struct s_reader *reader, ECM_REQUEST *er)
{
	if (!er) return;
	if (!reader) return;
	struct s_client *cl = reader->client;
	if (!check_client(cl)) return;

	mycs_trace(D_ADB, "myrdr:reader_get_ecm(%s.%d)", reader->label, er->rc);

	if (!chk_bcaid(er, &reader->ctab)) {
		rdr_log_dbg(reader, D_READER, "caid %04X filtered", er->caid);
		mycs_trace(D_READER, "caid %04X filtered", er->caid);
		write_ecm_answer(reader, er, E_NOTFOUND, E2_CAID, NULL, NULL, NULL);
		return;
	}

#if defined(MODULE_XCAS)
	if (IS_CONSTCW_READERS(reader))
	{
		if (er->constAfterwards > 1) {
			mycs_trace(D_ADB, "myrdr:constAfterwards.%s", reader->label);
			return;
		}
	}
#endif
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (CSREADER_ChkEcmSkipto(reader, er))
	{
		mycs_trace(D_ADB, "myrdr:ecm_useless.%s", reader->label);
		return;
	}
#endif

	// CHECK if ecm already sent to reader
	struct s_ecm_answer *ea_er = get_ecm_answer(reader, er);
	if (!ea_er) {
		mycs_trace(D_ADB, "myrdr:ecm_sent.");
		return;
	}

	struct s_ecm_answer  *ea = NULL, *ea_prev=NULL;
	struct ecm_request_t *ecm;
	time_t timeout;

	cs_readlock(&ecmcache_lock);
	for (ecm = ecmcwcache; ecm; ecm = ecm->next)
	{
		timeout = time(NULL) - ((cfg.ctimeout+500)/1000+1);
		if (ecm->tps.time <= timeout) break;
#if defined(MODULE_XCAS)
		if (er->constAfterwards)
		{
			mycs_trace(D_ADB, "myrdr:constAfterwards.");
			break;
		}
#endif

		if (!ecm->matching_rdr || ecm == er || ecm->rc == E_99) continue;

		// match same ecm
		if (er->caid == ecm->caid && !memcmp(er->ecmd5, ecm->ecmd5, CS_ECMSTORESIZE))
		{
			// check if ask this reader
			ea = get_ecm_answer(reader, ecm);
			if (ea && !ea->is_pending && (ea->status & REQUEST_SENT) && ea->rc!=E_TIMEOUT) { break; }
			ea = NULL;
		}
	}
	cs_readunlock(&ecmcache_lock);
	if (ea) // found ea in cached ecm, asking for this reader
	{
		ea_er->is_pending = true;

		cs_readlock(&ea->ecmanswer_lock);
		if (ea->rc < E_99)
		{
			cs_readunlock(&ea->ecmanswer_lock);
			cs_log_dbg(D_LB, "{client %s, caid %04X, prid %06X, srvid %04X} [reader_get_ecm] ecm already sent to reader %s (%s)", (check_client(er->client) ? er->client->account->usr : "-"), er->caid, er->prid, er->srvid, reader ? reader->label : "-", ea->rc==E_FOUND?"OK":"NOK");

			//e.g. we cannot send timeout, because "ea_temp->er->client" could wait/ask other readers! Simply set not_found if different from E_FOUND!
			write_ecm_answer(reader, er, (ea->rc==E_FOUND ? E_FOUND : E_NOTFOUND), ea->rcEx, ea->cw, &ea->cwEx, NULL);
			return;
		}
		else
		{
			ea_prev = ea->pending;
			ea->pending = ea_er;
			ea->pending->pending_next = ea_prev;
			cs_log_dbg(D_LB, "{client %s, caid %04X, prid %06X, srvid %04X} [reader_get_ecm] ecm already sent to reader %s... set as pending", (check_client(er->client) ? er->client->account->usr : "-"), er->caid, er->prid, er->srvid, reader ? reader->label : "-");
		}
		cs_readunlock(&ea->ecmanswer_lock);
		return;
	}

	lb_update_last(ea_er, reader);

	if (ecm_ratelimit_check(reader, er, 1) != OK) {
		rdr_log_dbg(reader, D_READER, "ratelimiter has no space left -> skip!");
		return;
	}

	if (is_cascading_reader(reader)) { // forward request to proxy reader
		cl->last_srvid= er->srvid;
		cl->last_caid = er->caid;
		casc_process_ecm(reader, er);
		cl->lastecm   = time((time_t *)0);
		return;
	}

	cardreader_process_ecm(reader, cl, er); // forward request to physical reader
}

void reader_do_card_info(struct s_reader *reader)
{
	if (!reader) return;
	cardreader_get_card_info(reader);

	if (reader->ph.c_card_info) {
		 reader->ph.c_card_info();
	}
}

void reader_do_idle(struct s_reader *reader)
{
	if (reader->ph.c_idle) { reader->ph.c_idle(); }
	else if (reader->tcp_ito > 0)
	{
		time_t  now;
		int32_t time_diff;
		time(&now);
		time_diff = llabs(now - reader->last_s);
		if (time_diff > reader->tcp_ito) {
			struct s_client *cl = reader->client;
			if (check_client(cl) && reader->tcp_connected && reader->ph.type == MOD_CONN_TCP)
			{
				rdr_log_dbg(reader, D_READER, "inactive_timeout, close connection (fd=%d)", cl->pfd);
				network_tcp_connection_close(reader, "inactivity");
			}
			else {
				reader->last_s = now;
			}
		}
	}
}

int32_t reader_init(struct s_reader *reader)
{
	if (!reader) return 0;
	if (!reader->client) return 0;
	struct s_client *client = reader->client;

	if (is_cascading_reader(reader))
	{
		client->typ  = 'p';
		client->port = reader->r_port;
		set_null_ip(&client->ip);

		if (!(reader->ph.c_init))
		{
			rdr_log(reader, "FATAL: protocol not supporting cascading");
			return 0;
		}

		if (reader->ph.c_init(client)) {
			// proxy reader start failed
			rdr_log(reader, "myrdr:init error{%s}", reader->label);
			return 0;
		}

		if (client->ecmtask)
		{
			add_garbage(client->ecmtask);
			client->ecmtask = NULL;
		}

		if (!cs_malloc(&client->ecmtask, cfg.max_pending * sizeof(ECM_REQUEST)))
			return 0;

		MYREADER_TRACE("myrdr:initialized(%s,%d)\n", reader->device, reader->r_port);
	}
	else {
		if (!reader->typ) return 0;
		if (!cardreader_init(reader)) return 0;
	}

	ll_destroy_data(&reader->emmstat);

	client->login = time((time_t *)0);
	client->init_done = 1;

	return 1;
}

#if !defined(WITH_CARDREADER) && defined(WITH_STAPI)
/* Dummy function stub for stapi compiles without cardreader as libstapi needs it. */
int32_t ATR_InitFromArray(ATR *atr, const unsigned char atr_buffer[ATR_MAX_SIZE], uint32_t length)
{
	(void)atr;
	(void)atr_buffer;
	(void)length;
	return 0;
}
#endif

void cs_card_info(void)
{
	struct s_client *cl;
	for (cl = first_client->next; cl ; cl = cl->next)
	{
		if (cl->typ == 'r' && cl->reader)
			add_job(cl, ACTION_READER_CARDINFO, NULL, 0);
	}
}


/* Adds a reader to the list of active readers so that it can serve ecms. */
static void add_reader_to_active(struct s_reader *rdr)
{
	struct s_reader *rdr2, *rdr_prv = NULL, *rdr_tmp = NULL;
	int8_t at_first = 1;

	if (rdr->next) {
		remove_reader_from_active(rdr);
	}
	cs_writelock(&readerlist_lock);
	cs_writelock(&clientlist_lock);

	// search configured position:
	LL_ITER itr  = ll_iter_create(configured_readers);
	while ((rdr2 = ll_iter_next(&itr)))
	{
		if (rdr2 == rdr) break;
		if (rdr2->client && rdr2->enable) {
			rdr_prv  = rdr2;
			at_first = 0;
		}
	}

	// insert at configured position:
	if (first_active_reader)
	{
		if (at_first) {
			rdr->next = first_active_reader;
			first_active_reader = rdr;
			//resort client list:
			struct s_client *prev, *cl;
			for (prev = first_client, cl = first_client->next;
					prev->next != NULL; prev = prev->next, cl = cl->next)
			{
				if (rdr->client == cl)
					break;
			}
			if (cl && rdr->client == cl) {
				prev->next = cl->next; //remove client from list
				cl->next = first_client->next;
				first_client->next = cl;
			}
		}
		else {
			for (rdr2=first_active_reader; rdr2->next && rdr2 != rdr_prv ; rdr2=rdr2->next) ; //search last element
			rdr_prv = rdr2;
			rdr_tmp = rdr2->next;
			rdr2->next = rdr;
			rdr->next = rdr_tmp;
			//resort client list:
			struct s_client *prev, *cl;
			for (prev = first_client, cl = first_client->next;
					prev->next != NULL; prev = prev->next, cl = cl->next)
			{
				if (rdr->client == cl)
					break;
			}
			if (cl && rdr->client == cl) {
				prev->next = cl->next; //remove client from list
				cl->next = rdr_prv->client->next;
				rdr_prv->client->next = cl;
			}
		}
	}
	else {
		first_active_reader = rdr;
	}
	rdr->active = 1;
	cs_writeunlock(&clientlist_lock);
	cs_writeunlock(&readerlist_lock);
}

/* Removes a reader from the list of active readers so that no ecms can be requested anymore. */
void remove_reader_from_active(struct s_reader *rdr)
{
	struct s_reader *rdr2, *prv = NULL;

	mycs_trace(D_ADB, "myrdr:REMOVE READER(%s) FROM ACTIVE", rdr->label);
	cs_writelock(&readerlist_lock);
	for (rdr2 = first_active_reader; rdr2; prv = rdr2, rdr2 = rdr2->next)
	{
		if (rdr2 == rdr)
		{
			if (prv) prv->next = rdr2->next;
			else first_active_reader = rdr2->next;
			break;
		}
	}
	rdr->next = NULL;
	rdr->active = 0;
	cs_writeunlock(&readerlist_lock);
}

/* Starts or restarts a cardreader without locking. If restart=1, the existing thread is killed before restarting,
   if restart=0 the cardreader is only started. */
static int32_t restart_cardreader_int(struct s_reader *rdr, int32_t restart)
{
	if (!rdr) return 0;
	struct s_client *cl = rdr->client;

	if (restart) {
		remove_reader_from_active(rdr); // remove from list
		kill_thread(cl);  // kill old thread
		cs_sleepms(1500); //we have to wait a bit so free_client is ended and socket closed too!
	}

	int timeouts = 0;
	while (restart && is_valid_client(cl)) {
		// If we quick disable+enable a reader (webif), remove_reader_from_active is called from
		// cleanup. this could happen AFTER reader is restarted, so oscam crashes or reader is hidden
		mycs_trace(D_ADB, "myrdr:WAITING FOR CLEANUP(%d)", timeouts);
		cs_sleepms(500);
		if (++timeouts>10) break;
	}

#if defined(WITH_HISILICON)
	// sky(sim)
	if (IS_CARD_READER(rdr)) {
		if (!chker_smartcard_enable()) {
			rdr_log(rdr, "smartcard nosupport\n");
			rdr->card_status = UNKNOWN;
			rdr->client = NULL;
			rdr->enable = 0;
			return 0;
		}
	}

	if (is_cascading_reader(rdr)) {
		if ( chker_factoy_products()) {
			rdr_log(rdr, "factoy products nosupport\n");
			rdr->card_status = UNKNOWN;
			rdr->client = NULL;
			rdr->enable = 0;
			return 0;
		}
	}
#endif

	mycs_trace(D_ADB, "myrdr:restart_cardreader{%02X}(%s,%d,%d) {%d}",
			rdr->typ, rdr->device, rdr->r_port, rdr->l_port, restart);
	rdr->client = NULL;
	rdr->tcp_connected 	= 0;
	rdr->card_status 		= UNKNOWN;
	rdr->tcp_block_delay = 100;
	// sky(n)
	rdr->restarting 		= restart;
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	rdr->iks_ecmRequired	= 0;
	rdr->iks_ecmSento		= 0;
	rdr->ics_chMonitor	= 0;
#endif
	cs_ftime(&rdr->tcp_block_connect_till);

	if (rdr->device[0] && is_cascading_reader(rdr)) {
		if (!rdr->ph.num) {
			rdr->enable = 0;
			rdr_log(rdr, "Protocol Support missing. (typ=%d)", rdr->typ);
			return 0;
		}
	}

	if (!rdr->enable) return 0;
	if ( rdr->device[0])
	{
		if (restart) {
			rdr_log(rdr, "Restarting reader");
		}
		cl = create_client(first_client->ip);
		if (cl == NULL) return 0;
		cl->reader = rdr;
		rdr_log(rdr, "creating thread for device %s", rdr->label);

		cl->sidtabs.ok = rdr->sidtabs.ok;
		cl->sidtabs.no = rdr->sidtabs.no;
		cl->lb_sidtabs.ok = rdr->lb_sidtabs.ok;
		cl->lb_sidtabs.no = rdr->lb_sidtabs.no;
		cl->grp = rdr->grp;

		rdr->client = cl;

		cl->typ = 'r';

		add_job(cl, ACTION_READER_INIT, NULL, 0);
		add_reader_to_active(rdr);

		return 1;
	}
	return 0;
}

/* Starts or restarts a cardreader with locking. If restart=1, the existing thread is killed before restarting,
   if restart=0 the cardreader is only started. */
int32_t restart_cardreader(struct s_reader *rdr, int32_t restart)
{
	cs_writelock(&system_lock);
	int32_t result = restart_cardreader_int(rdr, restart);
	cs_writeunlock(&system_lock);
	return (result);
}

void init_cardreader(void)
{
	cs_log_dbg(D_TRACE, "cardreader: Initializing");
	cs_writelock(&system_lock);
	struct s_reader *rdr;

	cardreader_init_locks();
	LL_ITER itr = ll_iter_create(configured_readers);
	while ((rdr = ll_iter_next(&itr)))
	{
		cs_log_dbg(D_TRACE, "cardreader: %s(%d)", rdr->label, rdr->enable);
		if (rdr->enable) {
			restart_cardreader_int(rdr, 0);
		}
	}

	load_stat_from_file();
	cs_writeunlock(&system_lock);
}

void kill_all_readers(void)
{
	struct s_reader *rdr;
	for (rdr = first_active_reader; rdr; rdr = rdr->next)
	{
		struct s_client *cl = rdr->client;

		if (!cl) continue;
		rdr_log(rdr, "Killing reader");
		kill_thread(cl);
	}
	first_active_reader = NULL;
}

int32_t reader_slots_available(struct s_reader *reader, ECM_REQUEST *er)
{
	if (ecm_ratelimit_check(reader, er, 0) != OK) { // check ratelimiter & cooldown -> in check mode: dont register srvid!!!
		return 0; // no slot free
	}
	else {
		return 1; // slots available!
	}
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
uint16_t
CSREADER_Chkstatus(struct s_reader *rdr)
{
	uint16_t	rdrstatus;

	if (!rdr) return (READERSTATUS_UNKNOWN);
	if (!rdr->enable) return (READERSTATUS_DISABLED);
	switch (rdr->card_status) {
		case NO_CARD: 			rdrstatus = READERSTATUS_DISCONNECT; break;
		case CARD_NEED_INIT:	rdrstatus = READERSTATUS_DISCONNECT; break;
		case CARD_FAILURE:  	rdrstatus = READERSTATUS_FAILURE; 	 break;
		case CARD_INSERTED:	rdrstatus = READERSTATUS_OK;			 break;
		case CARD_UNREGISTER:rdrstatus = READERSTATUS_NOTSUPPORT; break;
		case UNKNOWN:
		default:					rdrstatus = READERSTATUS_UNKNOWN; 	 break;
	}
	return (rdrstatus);
}

struct s_reader *
CSREADER_GetReaders(int32_t typ)
{
	struct s_reader *rdr;
	for (rdr = first_active_reader; rdr; rdr = rdr->next)
	{
		if (rdr->typ == typ) return (rdr);
	}
	return (NULL);
}

void
CSREADER_Restart(int32_t typ)
{
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	struct s_reader *rdr;

	rdr = CSREADER_ChkActive(typ);
	if (!rdr) return;
	if (!rdr->client) return;
	mycs_trace(D_ADB, "myrdr:reader restart{%x}", typ);
	add_job(rdr->client, ACTION_READER_RESTART, NULL, 0);
#endif
}

bool
CSREADER_IsFTAbiss(uint16_t degree, uint32_t frequency, uint16_t srvid, uint16_t vidpid)
{
#if defined(MODULE_CONSTCW)
	struct s_reader *rdr;
	uint32_t pfid;
	uint16_t edegree;

	pfid = cs_i2BCD(frequency);
	edegree = cs_i2BCD(degree);

	rdr = CSREADER_GetReaders(R_CONSTCW);
	if (!rdr) return 0;
	if (CONSTCW_IsFtaBisses(rdr,
				0x2600,
				pfid,
				srvid,
				vidpid,
				edegree)) return 1;
#endif
	return 0;
}

#if defined(MODULE_CONSTCW) || defined(MODULE_XCAS) || defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
struct s_reader *
CSREADER_ChkActive(int32_t typ)
{
	struct s_reader *rdr;

	rdr = CSREADER_GetReaders(typ);
	if (!rdr) return 0;
	if (!rdr->enable) return 0;
	if ( rdr->card_status != CARD_INSERTED) return 0;
	if (!check_client(rdr->client)) return 0;
	return (rdr);
}


void
CSREADER_ChCloser(int dmuxid, struct demux_s *demuxp, int32_t typ)
{
	struct s_reader *rdr;

	rdr = CSREADER_ChkActive(typ);
	if (!rdr) return;
	if (!rdr->client) return;
	struct s_client *cl = rdr->client;
	mycs_trace(D_ADB, "myrdr:ch.stop{%02X,%p}{%d}", typ, cl, dmuxid);
	if (rdr->ph.c_ChCloser) {
		 rdr->ph.c_ChCloser(cl, dmuxid);
	}
}


bool
CSREADER_ChRetuning(int dmuxid, struct demux_s *demuxp, int32_t typ)
{
	struct s_reader *rdr;

	rdr = CSREADER_ChkActive(typ);
	if (!rdr) return 0;
	if (!rdr->ics_chRetune) return 0;
	mycs_trace(D_ADB, "myrdr:ch.Retunes");
#if defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
	if (!CSREADER_ChSetting(dmuxid, demuxp, typ, CHCS_CASRETUNE)) return 0;
#endif
	return 1;
}

void
CSREADER_ChDescrambled(int32_t typ, int descramble)
{
	struct s_reader *rdr;

	rdr = CSREADER_ChkActive(typ);
	if (!rdr) return;
	rdr->ch_descramble = descramble;
}

#endif	// defined(MODULE_XCAMD) || defined(MODULE_MORECAM)
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//--------------------------------------------------------------------
