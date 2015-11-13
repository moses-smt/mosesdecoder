
// $Id$

#include	<stdarg.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>

#include	"cmd.h"

#ifdef WIN32
#		define popen	_popen
#		define pclose _pclose
#endif

typedef struct {
  enum CommandType Type;
  const char *Name,
        *ArgStr;
  void	*Val;
  const void *p;
} Cmd_T;

static const Enum_T	BoolEnum[] = {
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

static Cmd_T	cmds[MAXPARAM+1];
static const char *SepString = " \t\n";

/// Return cmd->p, as an int.
static int get_p_int(const Cmd_T *cmd)
{
  return *(const int *)cmd->p;
}

/// Return cmd->p, as a pointer to a null-terminated array of Enum_T.
static const Enum_T *get_p_enums(const Cmd_T *cmd)
{
  return (const Enum_T *)cmd->p;
}

/// Return cmd->p, as a pointer to a string.
static const char *get_p_char(const Cmd_T *cmd)
{
  return (const char *)cmd->p;
}

/// Return cmd->p, as a pointer to an array of two ints.
static const int *get_p_range(const Cmd_T *cmd)
{
  return (const int *)cmd->p;
}

/// Return cmd->Val, as a pointer to int.
static int *get_val_int_ptr(const Cmd_T *cmd)
{
  return (int *)cmd->Val;
}

/// Return the int at which cmd->Val points.
static int get_val_int(const Cmd_T *cmd)
{
  return *get_val_int_ptr(cmd);
}

/// Update the int at which cmd->Val points.
static void update_val_int(const Cmd_T *cmd, int value)
{
  *get_val_int_ptr(cmd) = value;
}

/// Return cmd->Val, as a pointer to double.
static double *get_val_double_ptr(const Cmd_T *cmd)
{
  return (double *)cmd->Val;
}

/// Return the double at which cmd->Val points.
static double get_val_double(const Cmd_T *cmd)
{
  return *get_val_double_ptr(cmd);
}

/// Return cmd->Val as a pointer to a string pointer.
static const char **get_val_char_ptr(const Cmd_T *cmd)
{
  return (const char **)cmd->Val;
}

/// Return the string pointer at which cmd->Val points.
static const char *get_val_char(const Cmd_T *cmd)
{
  return *get_val_char_ptr(cmd);
}

/// Update the string pointer at which cmd->Val points.
static void update_val_char(const Cmd_T *cmd, const char *s)
{
  *get_val_char_ptr(cmd) = s;
}

int DeclareParams(const char *ParName, ...)
{
  va_list		args;
  static int	ParamN = 0;

  va_start(args, ParName);
  for(; ParName;) {
    int c,
        j = 0;
    if(ParamN==MAXPARAM) {
      fprintf(stderr, "Too many parameters !!\n");
      break;
    }
    for(c=1; j<ParamN&&(c=strcmp(cmds[j].Name,ParName))<0; j++)
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
    cmds[j].Type = va_arg(args, enum CommandType);
    cmds[j].Val = va_arg(args, void *);
    switch(cmds[j].Type) {
    case CMDENUMTYPE:	/* get the pointer to Enum_T struct  */
      cmds[j].p = va_arg(args, void *);
      break;
    case CMDSUBRANGETYPE: {	/* get the two extremes		     */
      int *subrange = calloc(2, sizeof(int));
      cmds[j].p = subrange;
      subrange[0] = va_arg(args, int);
      subrange[1] = va_arg(args, int);
    }
    break;
    case CMDGTETYPE:	/* get lower or upper bound	     */
    case CMDLTETYPE: {
      int *value = calloc(1, sizeof(int));
      cmds[j].p = value;
      value[0] = va_arg(args, int);
    }
    break;
    case CMDSTRARRAYTYPE: {	/* get the separators string	     */
      const char *s = va_arg(args, const char *);
      cmds[j].p = (s ? strdup(s) : NULL);
    }
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
    ParName = va_arg(args, const char *);
  }
  cmds[ParamN].Name = NULL;
  va_end(args);
  return 0;
}

static char *GetLine(FILE *fp, int n, char *Line)
{
  int	offs=0;

  for(;;) {
    int j, l;
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

static void EnumError(const Cmd_T *cmd, const char *s)
{
  const Enum_T	*en;

  fprintf(stderr,
          "Invalid value \"%s\" for parameter \"%s\"\n", s, cmd->Name);
  fprintf(stderr, "Valid values are:\n");
  for(en=get_p_enums(cmd); en->Name; en++) {
    if(*en->Name) {
      fprintf(stderr, "    %s\n", en->Name);
    }
  }
  fprintf(stderr, "\n");
  exit(1);
}

static void GteError(const Cmd_T *cmd, int n)
{
  fprintf(stderr,
          "Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
  fprintf(stderr, "Valid values must be greater than or equal to  %d\n",
          get_p_int(cmd));
  exit(1);
}

static void LteError(const Cmd_T *cmd, int n)
{
  fprintf(stderr,
          "Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
  fprintf(stderr, "Valid values must be less than or equal to  %d\n",
          get_p_int(cmd));
  exit(1);
}

static void SubrangeError(const Cmd_T *cmd, int n)
{
  const int *subrange = get_p_range(cmd);
  fprintf(stderr,
          "Value %d out of range for parameter \"%s\"\n", n, cmd->Name);
  fprintf(stderr, "Valid values range from %d to %d\n",
          subrange[0], subrange[1]);
  exit(1);
}

static void SetEnum(Cmd_T *cmd, const char *s)
{
  const Enum_T	*en;

  for(en=get_p_enums(cmd); en->Name; en++) {
    if(*en->Name && !strcmp(s, en->Name)) {
      update_val_int(cmd, en->Idx);
      return;
    }
  }
  EnumError(cmd, s);
}

static void SetSubrange(Cmd_T *cmd, const char *s)
{
  int	n;
  const int *subrange = get_p_range(cmd);

  if(sscanf(s, "%d", &n)!=1) {
    fprintf(stderr,
            "Integer value required for parameter \"%s\"\n",
            cmd->Name);
    exit(1);
  }
  if(n < subrange[0] || n > subrange[1]) {
    SubrangeError(cmd, n);
  }
  update_val_int(cmd, n);
}

static void SetGte(Cmd_T *cmd, const char *s)
{
  int	n;

  if(sscanf(s, "%d", &n)!=1) {
    fprintf(stderr,
            "Integer value required for parameter \"%s\"\n",
            cmd->Name);
    exit(1);
  }
  if(n<get_p_int(cmd)) {
    GteError(cmd, n);
  }
  update_val_int(cmd, n);
}

static char **str2array(const char *s, const char *sep)
{
  const char *p;
  char **a;
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

static void SetStrArray(Cmd_T *cmd, const char *s)
{
  *(char***)cmd->Val = str2array(s, get_p_char(cmd));
}

static void SetLte(Cmd_T *cmd, const char *s)
{
  int	n;

  if(sscanf(s, "%d", &n)!=1) {
    fprintf(stderr,
            "Integer value required for parameter \"%s\"\n",
            cmd->Name);
    exit(1);
  }
  if(n > get_p_int(cmd)) {
    LteError(cmd, n);
  }
  update_val_int(cmd, n);
}

static void SetParam(Cmd_T *cmd, const char *s)
{
  if(!*s && cmd->Type != CMDSTRINGTYPE) {
    fprintf(stderr,
            "WARNING: No value specified for parameter \"%s\"\n",
            cmd->Name);
    return;
  }
  switch(cmd->Type) {
  case CMDDOUBLETYPE:
    if(sscanf(s, "%lf", get_val_double_ptr(cmd))!=1) {
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
    if(sscanf(s, "%d", get_val_int_ptr(cmd))!=1) {
      fprintf(stderr,
              "Integer value required for parameter \"%s\"\n",
              cmd->Name);
      exit(1);
    }
    break;
  case CMDSTRINGTYPE:
    update_val_char(cmd,
                    (strcmp(s, "<NULL>") && strcmp(s, "NULL"))
                    ? strdup(s)
                    : 0);
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
}

static int Scan(const char *ProgName, Cmd_T *cmds, char *Line)
{
  char	*q,
        *p;
  int	i,
      hl,
      HasToMatch = FALSE,
      c0,
      c;

  p = Line+strspn(Line, SepString);
  hl = strcspn(p, SepString);
  if(!hl) {
    return 0;
  }
  q = strchr(p, '/');
  if(q && q-p<hl) {
    *q = 0;
    if(strcmp(p, ProgName)) {
      *q = '/';
      return 0;
    }
    *q = '/';
    HasToMatch=TRUE;
    p = q+1;
  }
  hl = strcspn(p, SepString);
  if(!hl) {
    return 0;
  }
  c0 = p[hl];
  p[hl] = 0;
  for(i=0, c=1; cmds[i].Name&&(c=strcmp(cmds[i].Name, p))<0; i++)
    ;
  p[hl] = c0;

  if (c)
    return HasToMatch && c;

  SetParam(cmds+i, p+hl+strspn(p+hl, SepString));
  return 0;
}

static void PrintEnum(const Cmd_T *cmd, int ValFlag, FILE *fp)
{
  const Enum_T	*en;

  fprintf(fp, "%s", cmd->Name);
  if(ValFlag) {
    for(en=get_p_enums(cmd); en->Name; en++) {
      if(*en->Name && en->Idx==get_val_int(cmd)) {
        fprintf(fp, ": %s", en->Name);
      }
    }
  }
  fprintf(fp, "\n");
}

static void PrintStrArray(const Cmd_T *cmd, int ValFlag, FILE *fp)
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
}

static void PrintParam(const Cmd_T *cmd, int ValFlag, FILE *fp)
{
  fprintf(fp, "%4s", "");
  switch(cmd->Type) {
  case CMDDOUBLETYPE:
    fprintf(fp, "%s", cmd->Name);
    if(ValFlag) fprintf(fp, ": %22.15e", get_val_double(cmd));
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
    if(ValFlag) fprintf(fp, ": %d", get_val_int(cmd));
    fprintf(fp, "\n");
    break;
  case CMDSTRINGTYPE:
    fprintf(fp, "%s", cmd->Name);
    if(ValFlag) {
      const char *value = get_val_char(cmd);
      if(value) {
        fprintf(fp, ": \"%s\"", value);
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
}

static void PrintParams(int ValFlag, FILE *fp)
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
}

static void CmdError(const char *opt)
{
  fprintf(stderr, "Invalid option \"%s\"\n", opt);
  fprintf(stderr, "This program expectes the following parameters:\n");
  PrintParams(FALSE, stderr);
  exit(0);
}

int GetParams(int *n, char ***a, const char *CmdFileName)
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
      if(IsPipe) pclose(fp);
      else fclose(fp);
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
