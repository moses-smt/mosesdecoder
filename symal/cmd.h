
// $Id$

#if !defined(CMD_H)

#define	CMD_H

enum CommandType {
  CMDDOUBLETYPE = 1,
  CMDENUMTYPE,
  CMDINTTYPE,
  CMDSTRINGTYPE,
  CMDSUBRANGETYPE,
  CMDGTETYPE,
  CMDLTETYPE,
  CMDSTRARRAYTYPE,
  CMDBOOLTYPE
};

typedef struct {
  const char	*Name;
  int	Idx;
} Enum_T;

#ifdef  __cplusplus
extern "C" {
#endif

  int DeclareParams(const char *, ...);
  int GetParams(int *n, char ***a, const char *CmdFileName);

#ifdef  __cplusplus
}
#endif
#endif
