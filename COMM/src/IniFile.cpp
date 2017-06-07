#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "IniFile.h"
using namespace std;
void skip_left_space(char **str)
{
	while ((*(*str) == ' ') || (*(*str) == '\t'))
		(*str)++;
}

void skip_right_space(char **str)
{
	int i;
	i = strlen(*str);
	while (i >=0 && (*(*str + i-1) ==' ' || *(*str + i-1) =='\t'))
		i--;
	*(*str+i) =0;
}

void skip_lr_space(char **str)
{
	skip_left_space(str);
	skip_right_space(str);
}

int isapp(const char *linedata, const char *appname)
{
	char line[LINELEN];
	char *str = line;
	char *right;

	memcpy(line, linedata, LINELEN);

	skip_lr_space(&str);
	if (*str != '[')
		return -1;
	else
	{
		str++;
		right = str;
		while (*right != '\0')
		{
			if (*right == ']')
				break;
			right++;
		}
		if (*right != '\0')
			*right = '\0';
	}
	skip_lr_space(&str);
	if (strcasecmp(str, appname) == 0)
		return(0);
	return(-1);
}

int gotoapp(FILE * file, const char *appname)
{
	char line[LINELEN];
	char *str = NULL;

	while ((str = fgets(line, LINELEN, file)) != NULL)
	{
		if (isapp(str, appname) == 0)
			return 0;
	}

	return -1;
}

int notkeyline(char *line, const char *keyname, char **val)
{
	char *str = line;
	char *right;

	skip_left_space(&str);

	if (*str == '[') return -1;

	right = str;
	while (*right != '\0')
	{
		if (('=' == *right) || (' ' == *right)
			|| ('\t' == *right))
			break;
		right++;
	}
	if (*right != '\0')
	{
		*right = '\0';
	} else
		return 1;
	skip_lr_space(&str);
	if (strcasecmp(str, keyname) == 0 )
	{
		str = right + 1;
		skip_lr_space(&str);
		*val = str;
		return 0;
	}

	return 1;
}

int getkey(FILE * file, const char *keyname, char *buf, int len)
{
	char line[LINELEN];
	char *str = NULL;

	while (NULL != (str = fgets(line, LINELEN, file)))
	{
		int len = strlen(line);
		while (line[len - 1] == '\n' || line[len - 1] == '\r')
			len--;
		line[len] = '\0';
		if (!notkeyline(line, keyname, &str))
		{
			strncpy(buf, str, len);
			return 0;
		}
	}

	return -1;
}

void addappkey(FILE *file, const char *appname, const char *key,
			   const char *val)
{
	char buf[LINELEN] ="[";

	strncat(buf, appname, LINELEN-1);
	strcat(buf, "]");
	fprintf(file, "%s\n", buf);
	snprintf(buf, LINELEN, "%s=%s", key, val);
	fprintf(file, "%s\n", buf);
}



/* get a inifile's key value, returned by string */
int get_ini_string(const char *filename,const char *appname,
				   const char *keyname,const char *defval,
				   char *buf, int len)
{
	FILE *file = fopen(filename, "r+");

	if (file == NULL || buf == NULL) return -1;
	if (gotoapp(file, appname) < 0)
	{
		strncpy(buf, defval, len);
		return(-1);
	} 
	else
	{
		if (getkey(file, keyname, buf, len) < 0)
		{
			if (defval != NULL)
				strncpy(buf, defval, len);
			else
			{
				buf[0] = 0;
			return(-1);
			}
		}
	}

	fclose(file);

	return 0;
}

/* get a inifile's key value, returned by int */
int get_ini_int(const char *filename,const char *appname,
				const char *keyname,const int defval,
				int *val)
{
	char buf[LINELEN];

	get_ini_string(filename,appname,keyname, NULL,buf,LINELEN);
	if (buf != NULL)
		*val = atoi(buf);
	else
		*val = defval;

	return 0;
}

/* get a inifile's key value, returned by int */
int get_ini_bool(const char *filename,const char *appname,
				 const char *keyname,const bool defval,
				 bool *val)
{
	char buf[LINELEN];

	get_ini_string(filename,appname,keyname,"",
				   buf,LINELEN);

	if (strcasecmp(buf,"TRUE")==0 || strcasecmp(buf,"T")==0)
	{
		*val=true;
	} else if (strcasecmp(buf,"FALSE") == 0 || strcasecmp(buf,"F")==0)
	{
		*val=false;
	} else
	{
		*val = defval;
		return(-1);
	}

	return(0);

}

/* get a inifile's key value, returned by hex int */
int get_ini_hexInt(const char *filename,const char *appname,
		   const char *keyname,const int defval,
		   int *val)
{
	char buf[LINELEN];
	int nLen = 0;
	int nPow = 0;

	get_ini_string(filename,appname,keyname, NULL,buf,LINELEN);
	if (buf != NULL)
	{
		nLen = strlen(buf);

		for(int j=nLen; j>0; j--)
		{
			nPow = 1;
			for(int k=0; k<nLen-j; k++)
			{
				nPow *= 16;
			}
			*val += (buf[j-1]-'0')*nPow; 
		}
	}
	else
	{
		*val = defval;
	}

	return 0;	
}

/* set a inifile's key value by string */
int set_ini_string( const char *filename,const char *appname,
					const char *keyname,const char *val)
{
	int app,ret;
	FILE *file,*newfile;
	char *str,tmpname[513];
	char line[LINELEN];
	char linecpy[LINELEN];

	if ((file = fopen(filename, "r+")) == NULL)
	{

		if ((file = fopen(filename, "w+"))  == NULL)
			return -1;

		addappkey(file, appname, keyname, val);
		fclose(file);
	} else
	{
		sprintf(tmpname, "%s~", filename);
		app = 0;
		newfile = fopen(tmpname, "w+");
		while (fgets(line, LINELEN, file) != NULL)
		{
			if (app <= 0)
			{
				if (isapp(line, appname) == 0)
					app =1;
				fprintf(newfile, "%s", line);
			} else
			{
				memcpy(linecpy, line, LINELEN);
				if ((ret = notkeyline(line, keyname, &str)) == 0)
				{
					fprintf(newfile, "%s=%s\n", keyname, val);
					app = -1;
				} else if (ret == 1)
				{
					fprintf(newfile, "%s", linecpy);
				} else
				{
					fprintf(newfile, "%s=%s\n", keyname, val);
					fprintf(newfile, "%s", linecpy);
					app = -1;
				}
			}
		}
		if (app == 0)
		{
			addappkey(newfile, appname, keyname, val);
		}
		fclose(newfile);
		fclose(file);
		rename(tmpname, filename);
	}

	return 0;
}

/* set a inifile's key value by int */
int set_ini_int(const char *filename,const char *appname,
				const char *keyname,const int val)
{
	char buf[LINELEN];
	snprintf(buf,LINELEN,"%d",val);
	return(set_ini_string( filename,appname,keyname,buf));
}

/* set a inifile's key value by bool */
int set_ini_bool(const char *filename,const char *appname,
				 const char *keyname,const int val)
{
	char buf[LINELEN];

	if (val)
		snprintf(buf,LINELEN,"yes");
	else
		snprintf(buf,LINELEN,"no");

	return(set_ini_string( filename,appname,keyname,buf));
}
