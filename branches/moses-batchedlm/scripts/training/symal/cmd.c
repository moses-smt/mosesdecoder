
// $Id$

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>

#include	"cmd.h"

#ifdef WIN32
#		define popen	_popen
#		define pclose _pclose
#endif

static Enum_T	BoolEnum[] = {
	{	"FALSE",	0	},
	{	"TRUE",		1	},
	{	0,		0	}
};

#ifdef	NEEDSTRDUP
char	*strdup();
#endif

#define	FALSE	0
#define	TRUE	1

#define	LINSIZ		10240
#define	MAXPARAM	256

static char	*GetLine(),
		**str2array();
static int	Scan(),
		SetParam(),
		SetEnum(),
		SetSubrange(),
		SetStrArray(),
		SetGte(),
		SetLte(),
		CmdError(),
		EnumError(),
		SubrangeError(),
		GteError(),
		LteError(),
		PrintParam(),
		PrintEnum(),
		PrintStrArray();

static Cmd_T	cmds[MAXPARAM+1];
static char	*SepString = " \t\n";

#if defined(__STDC__)
#include	<stdarg.h>
int DeclareParams(char *ParName, ...)
#else
#include	<varargs.h>
int DeclareParams(ParName, va_alist)
char	*ParName;
va_dcl
#endif
{
	va_list		args;
	static int	ParamN = 0;
	int		j,
			c;
	char		*s;

#if defined(__STDC__)
	va_start(args, ParName);
#else
	va_start(args);
#endif
	for(;ParName;) {
		if(ParamN==MAXPARAM) {
			fprintf(stderr, "Too many parameters !!\n");
			break;
		}
		for(j=0,c=1; j<ParamN&&(c=strcmp(cmds[j].Name,ParName))<0; j++)
			;
		if(!c) {
			fprintf(stderr,
				"Warning: parameter \"%s\" declared twice.\n",
				ParName);
		}
		for(c=ParamN; c>j; c--) {
			cmds[c] = cmds[c-1];
		}
		cmds[j].Name = ParName;
		cmds[j].Type = va_arg(args, int);
		cmds[j].Val = va_arg(args, void *);
		switch(cmds[j].Type) {
		case CMDENUMTYPE:	/* get the pointer to Enum_T struct  */
			cmds[j].p = va_arg(args, void *);
			break;
		case CMDSUBRANGETYPE:	/* get the two extremes		     */
			cmds[j].p = (void*) calloc(2, sizeof(int));
			((int*)cmds[j].p)[0] = va_arg(args, int);
			((int*)cmds[j].p)[1] = va_arg(args, int);
			break;
		case CMDGTETYPE:	/* get lower or upper bound	     */
		case CMDLTETYPE:
			cmds[j].p = (void*) calloc(1, sizeof(int));
			((int*)cmds[j].p)[0] = va_arg(args, int);
			break;
		case CMDSTRARRAYTYPE:	/* get the separators string	     */
			cmds[j].p = (s=va_arg(args, char*))
				    ? (void*)strdup(s) : 0;
			break;
		case CMDBOOLTYPE:
			cmds[j].Type = CMDENUMTYPE;
			cmds[j].p = BoolEnum;
			break;
		case CMDDOUBLETYPE:	/* nothing else is needed	     */
		case CMDINTTYPE:
		case CMDSTRINGTYPE:
			break;
		default:
			fprintf(stderr, "%s: %s %d %s \"%s\"\n",
				"DeclareParam()", "Unknown Type",
				cmds[j].Type, "for parameter", cmds[j].Name);
			exit(1);
		}
		ParamN++;
		ParName = va_arg(args, char *);
	}
	cmds[ParamN].Name = NULL;
	va_end(args);
	return 0;
}

int GetParams(n, a, CmdFileName)
int	*n;
char	***a;
char	*CmdFileName;
{
	char	*Line,
		*ProgName;
	int	argc = *n;
	char	**argv = *a,
		*s;
	FILE	*fp;
	int	IsPipe;

#ifdef	MSDOS
#define	PATHSEP '\\'
	char	*dot = NULL;
#else
#define	PATHSEP '/'
#endif

	if(!(Line=malloc(LINSIZ))) {
		fprintf(stderr, "GetParams(): Unable to alloc %d bytes\n",
			LINSIZ);
		exit(1);
	}
	if((ProgName=strrchr(*argv, PATHSEP))) {
		++ProgName;
	} else {
		ProgName = *argv;
	}
#ifdef	MSDOS
	if(dot=strchr(ProgName, '.')) *dot = 0;
#endif
	--argc;
	++argv;
	for(;;) {
		if(argc && argv[0][0]=='-' && argv[0][1]=='=') {
			CmdFileName = argv[0]+2;
			++argv;
			--argc;
		}
		if(!CmdFileName) {
			break;
		}
		IsPipe = !strncmp(CmdFileName, "@@", 2);
		fp = IsPipe
		     ? popen(CmdFileName+2, "r")
		     : strcmp(CmdFileName, "-")
		       ? fopen(CmdFileName, "r")
		       : stdin;
		if(!fp) {
			fprintf(stderr, "Unable to open command file %s\n",
				CmdFileName);
			exit(1);
		}
		while(GetLine(fp, LINSIZ, Line) && strcmp(Line, "\\End")) {
			if(Scan(ProgName, cmds, Line)) {
				CmdError(Line);
			}
		}
		if(fp!=stdin) {
			if(IsPipe) pclose(fp); else fclose(fp);
		}
		CmdFileName = NULL;
	}
	while(argc && **argv=='-' && (s=strchr(*argv, '='))) {
		*s = ' ';
		sprintf(Line, "%s/%s", ProgName, *argv+1);
		*s = '=';
		if(Scan(ProgName, cmds, Line)) CmdError(*argv);
		--argc;
		++argv;
	}
	*n = argc;
	*a = argv;
#ifdef MSDOS
	if(dot) *dot = '.';
#endif
	free(Line);
	return 0;
}

int PrintParams(ValFlag, fp)
int	ValFlag;
FILE	*fp;
{
	int	i;

	fflush(fp);
	if(ValFlag) {
		fprintf(fp, "Parameters Values:\n");
	} else {
		fprintf(fp, "Parameters:\n");
	}
	for(i=0; cmds[i].Name; i++) PrintParam(cmds+i, ValFlag, fp);
	fprintf(fp, "\n");
	fflush(fp);
	return 0;
}

int SPrintParams(a, pfx)
char	***a,
	*pfx;
{
	int	l,
		n;
	Cmd_T	*cmd;

	if(!pfx) pfx="";
	l = strlen(pfx);
	for(n=0, cmd=cmds; cmd->Name; cmd++) n += !!cmd->ArgStr;
	a[0] = calloc(n, sizeof(char*));
	for(n=0, cmd=cmds; cmd->Name; cmd++) {
		if(!cmd->ArgStr) continue;
		a[0][n] = malloc(strlen(cmd->Name)+strlen(cmd->ArgStr)+l+2);
		sprintf(a[0][n], "%s%s=%s", pfx, cmd->Name, cmd->ArgStr);
		++n;
	}
	return n;
}

static int CmdError(opt)
char	*opt;
{
	fprintf(stderr, "Invalid option \"%s\"\n", opt);
	fprintf(stderr, "This program expectes the following parameters:\n");
	PrintParams(FALSE, stderr);
	exit(0);
}

static int PrintParam(cmd, ValFlag, fp)
Cmd_T	*cmd;
int	ValFlag;
FILE	*fp;
{
	fprintf(fp, "%4s", "");
	switch(cmd->Type) {
	case CMDDOUBLETYPE:
		fprintf(fp, "%s", cmd->Name);
		if(ValFlag) fprintf(fp, ": %22.15e", *(double *)cmd->Val);
		fprintf(fp, "\n");
		break;
	case CMDENUMTYPE:
		PrintEnum(cmd, ValFlag, fp);
		break;
	case CMDINTTYPE:
	case CMDSUBRANGETYPE:
	case CMDGTETYPE:
	case CMDLTETYPE:
		fprintf(fp, "%s", cmd->Name);
		if(ValFlag) fprintf(fp, ": %d", *(int *)cmd->Val);
		fprintf(fp, "\n");
		break;
	case CMDSTRINGTYPE:
		fprintf(fp, "%s", cmd->Name);
		if(ValFlag) {
			if(*(char **)cmd->Val) {
				fprintf(fp, ": \"%s\"", *(char **)cmd->Val);
			} else {
				fprintf(fp, ": %s", "NULL");
			}
		}
		fprintf(fp, "\n");
		break;
	case CMDSTRARRAYTYPE:
		PrintStrArray(cmd, ValFlag, fp);
		break;
	default:
		fprintf(stderr, "%s: %s %d %s \"%s\"\n",
			"PrintParam",
			"Unknown Type",
			cmd->Type,
			"for parameter",
			cmd->Name);
		exit(1);
	}
	return 0;
}

static char *GetLine(fp, n, Line)
FILE	*fp;
int	n;
char	*Line;
{
	int	j,
		l,
		offs=0;

	for(;;) {
		if(!fgets(Line+offs, n-offs, fp)) {
			return NULL;
		}
		if(Line[offs]=='#') continue;
		l = strlen(Line+offs)-1;
		Line[offs+l] = 0;
		for(j=offs; Line[j] && isspace(Line[j]); j++, l--)
			;
		if(l<1) continue;
		if(j > offs) {
			char	*s = Line+offs,
				*q = Line+j;

			while((*s++=*q++))
				;
		}
		if(Line[offs+l-1]=='\\') {
			offs += l;
			Line[offs-1] = ' ';
		} else {
			break;
		}
	}
	return Line;
}

static int Scan(ProgName, cmds, Line)
char	*ProgName,
	*Line;
Cmd_T	*cmds;
{
	char	*q,
		*p;
	int	i,
		hl,
		HasToMatch = FALSE,
		c0,
		c;

	p = Line+strspn(Line, SepString);
	if(!(hl=strcspn(p, SepString))) {
		return 0;
	}
	if((q=strchr(p, '/')) && q-p<hl) {
		*q = 0;
		if(strcmp(p, ProgName)) {
			*q = '/';
			return 0;
		}
		*q = '/';
		HasToMatch=TRUE;
		p = q+1;
	}
	if(!(hl = strcspn(p, SepString))) {
		return 0;
	}
	c0 = p[hl];
	p[hl] = 0;
	for(i=0, c=1; cmds[i].Name&&(c=strcmp(cmds[i].Name, p))<0; i++)
		;
	p[hl] = c0;
	if(!c) return SetParam(cmds+i, p+hl+strspn(p+hl, SepString));
	return HasToMatch && c;
}

static int SetParam(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	if(!*s && cmd->Type != CMDSTRINGTYPE) {
		fprintf(stderr,
			"WARNING: No value specified for parameter \"%s\"\n",
			cmd->Name);
		return 0;
	}
	switch(cmd->Type) {
	case CMDDOUBLETYPE:
		if(sscanf(s, "%lf", (double*)cmd->Val)!=1) {
			fprintf(stderr,
				"Float value required for parameter \"%s\"\n",
				cmd->Name);
			exit(1);
		}
		break;
	case CMDENUMTYPE:
		SetEnum(cmd, s);
		break;
	case CMDINTTYPE:
		if(sscanf(s, "%d", (int*)cmd->Val)!=1) {
			fprintf(stderr,
				"Integer value required for parameter \"%s\"\n",
				cmd->Name);
			exit(1);
		}
		break;
	case CMDSTRINGTYPE:
		*(char **)cmd->Val = (strcmp(s, "<NULL>") && strcmp(s, "NULL"))
				     ? strdup(s)
				     : 0;
		break;
	case CMDSTRARRAYTYPE:
		SetStrArray(cmd, s);
		break;
	case CMDGTETYPE:
		SetGte(cmd, s);
		break;
	case CMDLTETYPE:
		SetLte(cmd, s);
		break;
	case CMDSUBRANGETYPE:
		SetSubrange(cmd, s);
		break;
	default:
		fprintf(stderr, "%s: %s %d %s \"%s\"\n",
			"SetParam",
			"Unknown Type",
			cmd->Type,
			"for parameter",
			cmd->Name);
		exit(1);
	}
	cmd->ArgStr = strdup(s);
	return 0;
}

static int SetEnum(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	Enum_T	*en;

	for(en=(Enum_T *)cmd->p; en->Name; en++) {
		if(*en->Name && !strcmp(s, en->Name)) {
			*(int *) cmd->Val = en->Idx;
			return 0;
		}
	}
	return EnumError(cmd, s);
}

static int SetSubrange(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	int	n;

	if(sscanf(s, "%d", &n)!=1) {
		fprintf(stderr,
			"Integer value required for parameter \"%s\"\n",
			cmd->Name);
		exit(1);
	}
	if(n < *(int *)cmd->p || n > *((int *)cmd->p+1)) {
		return SubrangeError(cmd, n);
	}
	*(int *)cmd->Val = n;
	return 0;
}

static int SetGte(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	int	n;

	if(sscanf(s, "%d", &n)!=1) {
		fprintf(stderr,
			"Integer value required for parameter \"%s\"\n",
			cmd->Name);
		exit(1);
	}
	if(n<*(int *)cmd->p) {
		return GteError(cmd, n);
	}
	*(int *)cmd->Val = n;
	return 0;
}

static int SetStrArray(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	*(char***)cmd->Val = str2array(s, (char*)cmd->p);
	return 0;
}

static int SetLte(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	int	n;

	if(sscanf(s, "%d", &n)!=1) {
		fprintf(stderr,
			"Integer value required for parameter \"%s\"\n",
			cmd->Name);
		exit(1);
	}
	if(n > *(int *)cmd->p) {
		return LteError(cmd, n);
	}
	*(int *)cmd->Val = n;
	return 0;
}

static int EnumError(cmd, s)
Cmd_T	*cmd;
char	*s;
{
	Enum_T	*en;

	fprintf(stderr,
		"Invalid value \"%s\" for parameter \"%s\"\n", s, cmd->Name);
	fprintf(stderr, "Valid values are:\n");
	for(en=(Enum_T *)cmd->p; en->Name; en++) {
		if(*en->Name) {
			fprintf(stderr, "    %s\n", en->Name);
		}
	}
	fprintf(stderr, "\n");
	exit(1);
}

static int GteError(cmd, n)
Cmd_T	*cmd;
int	n;
{
	fprintf(stderr,
		"Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
	fprintf(stderr, "Valid values must be greater than or equal to  %d\n",
		*(int *)cmd->p);
	exit(1);
}

static int LteError(cmd, n)
Cmd_T	*cmd;
int	n;
{
	fprintf(stderr,
		"Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
	fprintf(stderr, "Valid values must be less than or equal to  %d\n",
		*(int *)cmd->p);
	exit(1);
}

static int SubrangeError(cmd, n)
Cmd_T	*cmd;
int	n;
{
	fprintf(stderr,
		"Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
	fprintf(stderr, "Valid values range from %d to %d\n",
		*(int *)cmd->p, *((int *)cmd->p+1));
	exit(1);
}

static int PrintEnum(cmd, ValFlag, fp)
Cmd_T	*cmd;
int	ValFlag;
FILE	*fp;
{
	Enum_T	*en;

	fprintf(fp, "%s", cmd->Name);
	if(ValFlag) {
		for(en=(Enum_T *)cmd->p; en->Name; en++) {
			if(*en->Name && en->Idx==*(int *)cmd->Val) {
				fprintf(fp, ": %s", en->Name);
			}
		}
	}
	fprintf(fp, "\n");
	return 0;
}

static int PrintStrArray(cmd, ValFlag, fp)
Cmd_T	*cmd;
int	ValFlag;
FILE	*fp;
{
	char	*indent,
		**s = *(char***)cmd->Val;
	int	l = 4+strlen(cmd->Name);

	fprintf(fp, "%s", cmd->Name);
	indent = malloc(l+2);
	memset(indent, ' ', l+1);
	indent[l+1] = 0;
	if(ValFlag) {
		fprintf(fp, ": %s", s ? (*s ? *s++ : "NULL") : "");
		if(s) while(*s) {
			fprintf(fp, "\n%s %s", indent, *s++);
		}
	}
	free(indent);
	fprintf(fp, "\n");
	return 0;
}

static char **str2array(s, sep)
char	*s,
	*sep;
{
	char	*p,
		**a;
	int	n = 0,
		l;

	if(!sep) sep = SepString;
	p = s += strspn(s, sep);
	while(*p) {
		p += strcspn(p, sep);
		p += strspn(p, sep);
		++n;
	}
	a = calloc(n+1, sizeof(char *));
	p = s;
	n = 0;
	while(*p) {
		l = strcspn(p, sep);
		a[n] = malloc(l+1);
		memcpy(a[n], p, l);
		a[n][l] = 0;
		++n;
		p += l;
		p += strspn(p, sep);
	}
	return a;
}
