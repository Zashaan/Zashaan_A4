/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Christopher Moretti & Zashaan Shaik                        */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "nodeFT.h"
#include "ft.h"

/*--------------------------------------------------------------------*/

/*
  A File Tree is a representation of a hierarchy of directories and
  files: the File Tree is rooted at a directory, directories
  may be internal nodes or leaves, and files are always leaves.
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

/*--------------------------------------------------------------------*/

/*
   Inserts a new directory into the FT with absolute path pcPath.
   Returns SUCCESS if the new directory is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertDir(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFirstNew = NULL;
    Node_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    /* validate pcPath and generate a Path_T for it */
    if(!bIsInitialized) return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    oNCurr = oNRoot;
    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }
    if(oNCurr != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNCurr)) break;
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            if(!Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            oNCurr = oNChild;
        }
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

        /* oNCurr is the node we're trying to insert */
        if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                        Node_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
        if(Node_isFile(oNCurr)) {
            Path_free(oPPath);
            return NOT_A_DIRECTORY;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while(ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        Node_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            if(oNFirstNew != NULL) (void) Node_free(oNFirstNew);
            return iStatus;
        }

        /* insert the new node for this level */
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL) (void) Node_free(oNFirstNew);
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL) oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update DT state variables to reflect insertion */
    if(oNRoot == NULL) oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
  Returns TRUE if the FT contains a directory with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsDir(const char *pcPath) {
   int iStatus;
   size_t ulSize;
   boolean bFile;

   assert(pcPath != NULL);

   iStatus = FT_stat(pcPath, &bFile, &ulSize);
   if (iStatus != SUCCESS) return FALSE;
   return !bFile;
}

/*--------------------------------------------------------------------*/

/*
  Removes the FT hierarchy (subtree) at the directory with absolute
  path pcPath. Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_DIRECTORY if pcPath is in the FT as a file not a directory
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmDir(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    if(!bIsInitialized) return INITIALIZATION_ERROR;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return iStatus;

    if (oNRoot == NULL) {
        Path_free(oPPath);
        return NO_SUCH_PATH;
    }

    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    /* find the closest ancestor of oPPath already in the tree */
    oNFound = oNRoot;
    if(oNFound != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNFound)) {
                Path_free(oPPath);
                return NO_SUCH_PATH;
            }
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            if(!Node_hasChild(oNFound, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                Path_free(oPPath);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNFound, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            oNFound = oNChild;
        }
    }

    Path_free(oPPath);
    
    if(Node_isFile(oNFound)) return NOT_A_DIRECTORY;
    ulCount -= Node_free(oNFound);
    if(ulCount == 0)
        oNRoot = NULL;

    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
   Inserts a new file into the FT with absolute path pcPath, with
   file contents pvContents of size ulLength bytes.
   Returns SUCCESS if the new file is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath,
                      or if the new file would be the FT root
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFirstNew = NULL;
    Node_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    /* validate pcPath and generate a Path_T for it */
    if(!bIsInitialized) return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    oNCurr = oNRoot;
    /* first checks if they even share something similar */
    if (oNRoot != NULL && 
        Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }
    if(oNCurr != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNCurr)) break;
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            if(!Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            oNCurr = oNChild;
        }
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

        /* oNCurr is the node we're trying to insert */
        if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                        Node_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
        if(Node_isFile(oNCurr)) {
            Path_free(oPPath);
            return NOT_A_DIRECTORY;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while(ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        Node_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            if(oNFirstNew != NULL) (void) Node_free(oNFirstNew);
            return iStatus;
        }

        /* insert the new node for this level */
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, TRUE);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL) (void) Node_free(oNFirstNew);
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL) oNFirstNew = oNCurr;
        ulIndex++;
    }

    /* add the replace contents thing here */

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if(oNRoot == NULL) oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    return SUCCESS;

}

/*--------------------------------------------------------------------*/

/*
  Returns TRUE if the FT contains a file with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsFile(const char *pcPath) {
    int iStatus;
    size_t ulSize;
    boolean bFile;

    assert(pcPath != NULL);

    iStatus = FT_stat(pcPath, &bFile, &ulSize);
    if (iStatus != SUCCESS) return FALSE;
    return bFile;
}

/*--------------------------------------------------------------------*/

/*
  Removes the FT file with absolute path pcPath.
  Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_FILE if pcPath is in the FT as a directory not a file
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmFile(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    if(!bIsInitialized) return INITIALIZATION_ERROR;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return iStatus;

    if (oNRoot == NULL) {
        Path_free(oPPath);
        return NO_SUCH_PATH;
    }

    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    /* find the closest ancestor of oPPath already in the tree */
    oNFound = oNRoot;
    if(oNFound != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNFound)) {
                Path_free(oPPath);
                return NO_SUCH_PATH;
            }
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            if(!Node_hasChild(oNFound, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                Path_free(oPPath);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNFound, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            oNFound = oNChild;
        }
    }

    Path_free(oPPath);
    
    if(!Node_isFile(oNFound)) return NOT_A_FILE;
    ulCount -= Node_free(oNFound);
    if(ulCount == 0)
        oNRoot = NULL;

    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
  Returns the contents of the file with absolute path pcPath.
  Returns NULL if unable to complete the request for any reason.

  Note: checking for a non-NULL return is not an appropriate
  contains check, because the contents of a file may be NULL.
*/
void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    if(!bIsInitialized) return NULL;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return NULL;

    if (oNRoot == NULL) {
        Path_free(oPPath);
        return NULL;
    }

    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return NULL;
    }

    /* find the closest ancestor of oPPath already in the tree */
    oNFound = oNRoot;
    if(oNFound != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNFound)) {
                Path_free(oPPath);
                return NULL;
            }
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return NULL;
            }
            if(!Node_hasChild(oNFound, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                Path_free(oPPath);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNFound, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return NULL;
            }
            oNFound = oNChild;
        }
    }

    Path_free(oPPath);
    
    if(!Node_isFile(oNFound)) return NULL;
    return Node_getContents(oNFound);
}

/*--------------------------------------------------------------------*/

/*
  Replaces current contents of the file with absolute path pcPath with
  the parameter pvNewContents of size ulNewLength bytes.
  Returns the old contents if successful. (Note: contents may be NULL.)
  Returns NULL if unable to complete the request for any reason.
*/
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength){
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);

    if(!bIsInitialized) return NULL;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return NULL;

    if (oNRoot == NULL) {
        Path_free(oPPath);
        return NULL;
    }

    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return NULL;
    }

    /* find the closest ancestor of oPPath already in the tree */
    oNFound = oNRoot;
    if(oNFound != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNFound)) {
                Path_free(oPPath);
                return NULL;
            }
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return NULL;
            }
            if(!Node_hasChild(oNFound, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                Path_free(oPPath);
                break;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNFound, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return NULL;
            }
            oNFound = oNChild;
        }
    }

    Path_free(oPPath);
    
    if(!Node_isFile(oNFound)) return NULL;
    return Node_replaceContents(oNFound, pvNewContents, ulNewLength);
}

/*--------------------------------------------------------------------*/

/*
  Returns SUCCESS if pcPath exists in the hierarchy,
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * MEMORY_ERROR if memory could not be allocated to complete request

  When returning SUCCESS,
  if path is a directory: sets *pbIsFile to FALSE, *pulSize unchanged
  if path is a file: sets *pbIsFile to TRUE, and
                     sets *pulSize to the length of file's contents

  When returning another status, *pbIsFile and *pulSize are unchanged.
*/
int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    size_t i;
    size_t ulChildID;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    if(!bIsInitialized) return INITIALIZATION_ERROR;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) return iStatus;

    if (oNRoot == NULL) {
        Path_free(oPPath);
        return NO_SUCH_PATH;
    }

    /* first checks if they even share something similar */
    if (oNRoot != NULL && Path_getSharedPrefixDepth(Node_getPath(oNRoot), oPPath) == 0) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    /* find the closest ancestor of oPPath already in the tree */
    oNFound = oNRoot;
    if(oNFound != NULL) {
        for(i = 2; i<= Path_getDepth(oPPath); i++) {
            Path_T oPPrefix = NULL;
            Node_T oNChild = NULL;

            if(Node_isFile(oNFound)) {
                Path_free(oPPath);
                return NO_SUCH_PATH;
            }
            iStatus = Path_prefix(oPPath, i, &oPPrefix);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            if(!Node_hasChild(oNFound, oPPrefix, &ulChildID)) {
                Path_free(oPPrefix);
                Path_free(oPPath);
                return NO_SUCH_PATH;
            }
            Path_free(oPPrefix);
            iStatus = Node_getChild(oNFound, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                Path_free(oPPath);
                return iStatus;
            }
            oNFound = oNChild;
        }
    }
    Path_free(oPPath);

    /* updates the values the pointers point to with the is file and
       get size values */
    *pbIsFile = Node_isFile(oNFound);
    if(*pbIsFile) *pulSize = Node_getSize(oNFound);

    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
  Sets the FT data structure to an initialized state.
  The data structure is initially empty.
  Returns INITIALIZATION_ERROR if already initialized,
  and SUCCESS otherwise.
*/
int FT_init(void) {
    if (bIsInitialized) return INITIALIZATION_ERROR;
    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;
    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
  Removes all contents of the data structure and
  returns it to an uninitialized state.
  Returns INITIALIZATION_ERROR if not already initialized,
  and SUCCESS otherwise.
*/
int FT_destroy(void) {
    if(!bIsInitialized) return INITIALIZATION_ERROR;
    if(oNRoot != NULL) Node_free(oNRoot);
    oNRoot = NULL;
    ulCount = 0;
    bIsInitialized = FALSE;
    return SUCCESS;
}

/*--------------------------------------------------------------------*/

/*
  Returns a string representation of the
  data structure, or NULL if the structure is
  not initialized or there is an allocation error.

  The representation is depth-first with files
  before directories at any given level, and nodes
  of the same type ordered lexicographically.

  Allocates memory for the returned string,
  which is then owned by client!
*/
char *FT_toString(void) {
    DynArray_T oDToDo;
    DynArray_T oDNodes;
    Node_T oNCurrent;
    Node_T oNChild;
    int iStatus;
    char *pcReturnString;
    char *pcReturnEditor;
    size_t i;
    size_t ulVisited = 0;
    size_t ulRemaining = 0;
    size_t ulTotalSize = 0;

    if (!bIsInitialized) return NULL;

    /* initializes the arrays */
    oDToDo = DynArray_new(ulCount);
    if (oDToDo == NULL) return NULL;
    oDNodes = DynArray_new(ulCount);
    if (oDNodes == NULL) {
        DynArray_free(oDToDo);
        return NULL;
    }
    if(oNRoot != NULL) {
        DynArray_set(oDToDo, ulRemaining, oNRoot);
        ulRemaining++;
    }

    /* goes through all the nodes and properly puts them in the array */
    while (ulRemaining != 0) {
        /* remove last node and record what it is */
        ulRemaining--;
        oNCurrent = DynArray_get(oDToDo, ulRemaining);
        DynArray_set(oDNodes, ulVisited, oNCurrent);
        ulVisited++;
        ulTotalSize += Path_getStrLength(Node_getPath(oNCurrent));

        /* does the directories in reverse order and then files in
           reverse and then assembles the correct order by finishing
           each directory first before another, etc. */
        for(i = Node_getNumChildren(oNCurrent); i > 0; i--) {
            iStatus = Node_getChild(oNCurrent, i - 1, &oNChild);
            assert(iStatus == SUCCESS);
            if(!Node_isFile(oNChild)) {
                DynArray_set(oDToDo, ulRemaining, oNChild);
                ulRemaining++;
            }
        }
        for(i = Node_getNumChildren(oNCurrent); i > 0; i--) {
            iStatus = Node_getChild(oNCurrent, i - 1, &oNChild);
            assert(iStatus == SUCCESS);
            if(Node_isFile(oNChild)) {
                DynArray_set(oDToDo, ulRemaining, oNChild);
                ulRemaining++;
            }
        }
    }
    DynArray_free(oDToDo);

    pcReturnString = malloc(ulTotalSize + 1);
    if(pcReturnString == NULL) {
        DynArray_free(oDNodes);
        return NULL;
    }
    pcReturnEditor = pcReturnString + ulTotalSize + 1;
    *pcReturnEditor = '\0';
    pcReturnEditor = pcReturnString;

    /* goes through and actually populates the string now */
    for(i = 0; i < ulVisited; i++) {
        oNCurrent = DynArray_get(oDNodes, i);
        strcat(pcReturnEditor, Path_getPathname(Node_getPath(oNCurrent)));
        strcat(pcReturnEditor, "\n");
    }
    DynArray_free(oDNodes);
    return pcReturnString;
}

/*--------------------------------------------------------------------*/