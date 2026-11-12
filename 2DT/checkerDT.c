/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"



/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      size_t ulDepthChild;
      size_t ulDepthParent;

      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);
      ulDepthChild = Path_getDepth(oPNPath);
      ulDepthParent = Path_getDepth(oPPPath);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }

      /* also checks that each child is one step deeper than their 
      parent */
      if (ulDepthChild == 0 || ulDepthParent != ulDepthChild - 1) {
         fprintf(stderr, "parent depth is %zu but child depth is %zu\n",
            ulDepthParent, ulDepthChild);
         return FALSE;
      }
   }

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;
   Node_T oNPreviousChild;

   oNPreviousChild = NULL;

   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* checks that each of the children is not NULL */
         if (oNChild == NULL) {
            fprintf(stderr, "child of index %zu is NULL\n", ulIndex);
            return FALSE;
         }

         /* checks that the parent of each child is correct */
         if (Node_getParent(oNChild) != oNNode) {
            fprintf(stderr, 
               "child of index %zu points to wrong parent\n", 
               ulIndex);
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized)
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }


   /* check that if the tree has no root node then count is zero */
   if (oNRoot == NULL && ulCount != 0) {
      fprintf(stderr, "tree has no root but count is %zu\n", ulCount);
      return FALSE;
   }

   /* checks that the root does not have a parent */
   if (oNRoot != NULL && (oNRoot) != NULL) {
      fprintf(stderr, "root node has a parent\n");
      return FALSE;
   }

   /* checks that the nodes that can be reached from the root is equal
      to the count */
   if (Node_getNumChildren(oNRoot) != ulCount - 1) {
      fprintf(stderr, "root node has %zu children but count is %zu\n",
         Node_getNumChildren(oNRoot), ulCount);
      return FALSE;
   }

   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
}
