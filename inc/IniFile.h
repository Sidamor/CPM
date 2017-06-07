#ifndef _INI_FILE_
#define _INI_FILE_

#define LINELEN	4096

#define INI_READ_STRING(mi, pn, mv) \
	get_ini_string(mi.szIniFileName , mi.szModName, pn, NULL, mv, sizeof(mv)) 

#define INI_READ_INT(mi, pn, mv) \
	get_ini_int(mi.szIniFileName , mi.szModName, pn, 0, (int *)&(mv))
	
#define INI_READ_BOOL(mi, pn, mv) \
	get_ini_bool(mi.szIniFileName , mi.szModName, pn, true, &(mv))

#define INI_SET_STRING(mi, pn, mv) \
	set_ini_string(mi.szIniFileName , mi.szModName, pn, mv)

#define INI_SET_INT(mi, pn, mv) \
	set_ini_int(mi.szIniFileName , mi.szModName, pn, mv)

#define INI_SET_BOOL(mi, pn, mv) \
	set_ini_bool(mi.szIniFileName , mi.szModName, pn, mv)

//filename为ini文件名称，appname为section名称
extern int get_ini_string(const char *filename,const char *appname,
		   const char *keyname,const char *defval,
		   char *buf, int len);
extern int get_ini_int(const char *filename,const char *appname,
		   const char *keyname,const int defval,
		   int *val);
extern int get_ini_bool(const char *filename,const char *appname,
		   const char *keyname,const bool defval,
		   bool *val);
extern int get_ini_hexInt(const char *filename,const char *appname,
		   const char *keyname,const int defval,
		   int *val);

extern int set_ini_string( const char *filename,const char *appname,
		    const char *keyname,const char *val);
extern int set_ini_int(const char *filename,const char *appname,
		    const char *keyname,const int val);
extern int set_ini_bool(const char *filename,const char *appname,
		    const char *keyname,const int val);

#endif
