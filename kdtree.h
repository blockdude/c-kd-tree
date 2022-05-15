/*
 * this is only for k-dimensions
 * and has no self balancing
 * properties so performace
 * may degrade during use.
 */

#ifndef KD_TREE
#define KD_TREE

#ifndef KD_DATA_TYPE
#define KD_DATA_TYPE int
#endif

typedef struct kd_tree kd_tree;

kd_tree *new_kd_tree    ( int k, void ( *free_item )( void * ) );

// getters
int kd_size             ( kd_tree *tree );
int kd_dim              ( kd_tree *tree );

// build tools
void *kd_replace        ( kd_tree *tree, KD_DATA_TYPE point[], void *item );
void *kd_insert         ( kd_tree *tree, KD_DATA_TYPE point[], void *item );
void *kd_remove         ( kd_tree *tree, KD_DATA_TYPE point[] );
int kd_delete           ( kd_tree *tree, KD_DATA_TYPE point[] );

// query tools
void **kd_query_range   ( kd_tree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE range, int *length );
void **kd_query_dim     ( kd_tree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE dim[], int *length );

// search tools
void *kd_search         ( kd_tree *tree, KD_DATA_TYPE point[] );
void *kd_nearest        ( kd_tree *tree, KD_DATA_TYPE point[] );

// clean up
void kd_free            ( kd_tree *tree );

#endif
