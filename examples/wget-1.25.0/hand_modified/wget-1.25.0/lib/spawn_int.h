/* Copyright (C) 2000, 2008-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <sys/types.h>

/* Data structure to contain the action information.  */
struct __spawn_action
{
  enum
  {
    spawn_do_close,
    spawn_do_dup2,
    spawn_do_open,
    spawn_do_chdir,
    spawn_do_fchdir
  } tag;

  union
  {
    struct
    {
      int fd;
    } close_action;
    struct
    {
      int fd;
      int newfd;
    } dup2_action;
    struct
    {
      int fd;
      char *path;
      int oflag;
      mode_t mode;
    } open_action;
    struct
    {
      char *path;
    } chdir_action;
    struct
    {
      int fd;
    } fchdir_action;
  } action;
};

#if defined(__cplusplus) && !_LIBC
/* gl_posix_spawn_file_actions_realloc() won't be available in our C++ build, so we'll just use the actual implementation from glibc (2.42) to define __posix_spawn_file_actions_realloc(): */

#include <stdlib.h>
#include <errno.h>
/* Function used to increase the size of the allocated array.  This
   function is called from the `add'-functions.  */
inline int
__posix_spawn_file_actions_realloc (posix_spawn_file_actions_t *file_actions)
{
  int newalloc = file_actions->__allocated + 8;
  void *newmem = realloc (file_actions->__actions,
              newalloc * sizeof (struct __spawn_action));
 
  if (newmem == NULL)
    /* Not enough memory.  */
    return ENOMEM;
 
  file_actions->__actions = (struct __spawn_action *) newmem;
  file_actions->__allocated = newalloc;
 
  return 0;
}

#else /* defined(__cplusplus) && !_LIBC */

#if !_LIBC
# define __posix_spawn_file_actions_realloc gl_posix_spawn_file_actions_realloc
#endif
extern int __posix_spawn_file_actions_realloc (posix_spawn_file_actions_t *
                                               file_actions);

#endif /* defined(__cplusplus) && !_LIBC */

#if defined(__cplusplus) && !_LIBC
/* gl_posix_spawn_internal() won't be available in our C++ build, so we'll just define __spawni() as a wrapper for posix_spawn(): */

#include <spawn.h>
/* Spawn a new process executing PATH with the attributes describes in *ATTRP.
   Before running the process perform the actions described in FILE-ACTIONS. */
inline int
__spawni (pid_t * pid, const char *file,
      const posix_spawn_file_actions_t * acts,
      const posix_spawnattr_t * attrp, const char *const argv[],
      const char *const envp[], int xflags)
{
  typedef char *const* argv_t;
  typedef char *const* envp_t;
  return posix_spawn (pid, file, acts, attrp, (argv_t)argv, (envp_t)envp);
}

#else /* defined(__cplusplus) && !_LIBC */

#if !_LIBC
# define __spawni gl_posix_spawn_internal
#endif
extern int __spawni (pid_t *pid, const char *path,
                     const posix_spawn_file_actions_t *file_actions,
                     const posix_spawnattr_t *attrp, const char *const argv[],
                     const char *const envp[], int use_path);
                     
#endif /* defined(__cplusplus) && !_LIBC */

