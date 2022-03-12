/*
 * this is only for k-dimensions
 * and has no self balancing
 * properties so performace
 * may degrade during use.
 */

#ifndef KD_TREE
#define KD_TREE

typedef struct kd_tree kd_tree;
typedef struct kd_result kd_result;

kd_tree *new_kd_tree    ( int k, void ( *free_item )( void *item ) );

// getters
int kd_size             ( kd_tree *tree );
int kd_dim              ( kd_tree *tree );

// build tools
int kd_insert           ( kd_tree *tree, int point[], void *item );
int kd_remove           ( kd_tree *tree, int point[] );
void *kd_pull           ( kd_tree *tree, int point[] );

// query tools
void **kd_query_range   ( kd_tree *tree, int point[], int range, int *length );
void **kd_query_dim     ( kd_tree *tree, int point[], int dim[], int *length );

// search tools
void *kd_search         ( kd_tree *tree, int point[] );
void *kd_nearest        ( kd_tree *tree, int point[] );

// clean up
void kd_free            ( kd_tree *tree );

#endif
