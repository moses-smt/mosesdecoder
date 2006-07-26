

#pragma once

#ifdef WIN32

inline const char *index(const char *str, char search)
{
	int i=0;
	while (i< strlen(str) ){
		if (str[i]==search) return &str[i];	
	}
	return NULL;
}


#endif