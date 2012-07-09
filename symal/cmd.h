
// $Id$

#if !defined(CMD_H)

#define	CMD_H

#define	CMDDOUBLETYPE	1
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
  char	*Name,
        *ArgStr;
  void	*Val,
        *p;
} Cmd_T;

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(__STDC__)
  int DeclareParams(char *, ...);
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



