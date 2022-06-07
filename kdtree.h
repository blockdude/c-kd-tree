#ifndef KDTREE_H
#define KDTREE_H

#define KDT_DATA_TYPE float

struct KDTree;

struct KDTree *new_kdtree   ( int k, void ( *free_item )( void * ) );
void free_kdtree            ( struct KDTree *tree );

// getters
int kdt_size                ( struct KDTree *tree );
int kdt_dim                 ( struct KDTree *tree );

// build tools
void *kdt_replace           ( struct KDTree *tree, KDT_DATA_TYPE point[], void *item ); // can return item on insertion or returns existing item on replacement
void *kdt_insert            ( struct KDTree *tree, KDT_DATA_TYPE point[], void *item ); // can return item on success or returns existing item on failure
void *kdt_remove            ( struct KDTree *tree, KDT_DATA_TYPE point[] ); // returns node item if point is found otherwise returns NULL
int kdt_delete              ( struct KDTree *tree, KDT_DATA_TYPE point[] ); // returns 1 on success and 0 on failure

// query tools
void kdt_query_range_func   ( struct KDTree *tree, KDT_DATA_TYPE point[], KDT_DATA_TYPE range, void ( *func )( void * ) );
void **kdt_query_range      ( struct KDTree *tree, KDT_DATA_TYPE point[], KDT_DATA_TYPE range, int *length );

void kdt_query_dim_func     ( struct KDTree *tree, KDT_DATA_TYPE point[], KDT_DATA_TYPE dim[], void ( *func )( void * ) );
void **kdt_query_dim        ( struct KDTree *tree, KDT_DATA_TYPE point[], KDT_DATA_TYPE dim[], int *length );

// search tools
void *kdt_search            ( struct KDTree *tree, KDT_DATA_TYPE point[] );
void *kdt_nearest           ( struct KDTree *tree, KDT_DATA_TYPE point[] );

#endif
