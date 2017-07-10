#ifndef MODULES_H_
#define MODULES_H_

void module_monitor(struct s_module *);
void module_camd35(struct s_module *);
void module_camd35_tcp(struct s_module *);
void module_camd33(struct s_module *);
void module_newcamd(struct s_module *);
void module_radegast(struct s_module *);
void module_serial(struct s_module *);
void module_cccam(struct s_module *);
void module_pandora(struct s_module *);
void module_scam(struct s_module *);
void module_ghttp(struct s_module *);
void module_gbox(struct s_module *);
void module_constcw(struct s_module *);
void module_csp(struct s_module *);
void module_dvbapi(struct s_module *);

#if defined(MODULE_XCAS)
void MODULE_xcas(struct s_module *);
#endif	// #if defined(MODULE_XCAMD)
#if defined(MODULE_XCAMD)
void MODULE_xcamd(struct s_module *);
#endif	// #if defined(MODULE_XCAMD)
#if defined(MODULE_MORECAM)
void MODULE_morecam(struct s_module *);
#endif	// #if defined(MODULE_MORECAM)

#endif
