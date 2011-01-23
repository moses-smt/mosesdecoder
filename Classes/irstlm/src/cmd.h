// $Id: cmd.h 3626 2010-10-07 11:41:05Z bertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/

#if !defined(CMD_H)

#define	CMD_H

#define	CMDDOUBLETYPE	1
#define	CMDFLOATTYPE	10
#define	CMDENUMTYPE	2
#define	CMDINTTYPE	3
#define	CMDSTRINGTYPE	4
#define	CMDSUBRANGETYPE	5
#define	CMDGTETYPE	6
#define	CMDLTETYPE	7
#define	CMDSTRARRAYTYPE	8
#define	CMDBOOLTYPE	9

typedef struct {
	const char	*Name;
	int	Idx;
} Enum_T;

typedef struct {
	int	Type;
	const char	*Name,
		*ArgStr;
	void	*Val,
		*p;
} Cmd_T;

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(__STDC__)
int DeclareParams(const char *, ...);
#else
int DeclareParams();
#endif

int	GetParams(int *n, char ***a,char *CmdFileName),
	SPrintParams(),
	PrintParams();

#ifdef  __cplusplus
}
#endif
#endif



