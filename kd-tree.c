#include <stdlib.h>
#include "kd-tree.h"

static void free_node( kd_node *node );

static int ptcmp( int size, int p1[], int p2[] )
{
    for ( int i = 0; i < size; i++ )
        if ( p1[ i ] != p2[ i ] )
            return 0;

    return 1;
}

kd_tree *new_kd_tree( int k, void ( *free_item )( void *item ) )
{
    if ( k < 2 )
        return NULL;

    kd_tree *tree = ( kd_tree * ) malloc( sizeof( kd_tree ) );

    if ( tree == NULL )
        return NULL;

    tree->root = NULL;
    tree->size = 0;
    tree->k = k;
    tree->free_item = free_item == NULL ? free : free_item;

    return tree;
}

kd_node *new_kd_node( int k, int point[], void *item )
{
    if ( k < 2 )
        return NULL;

    kd_node *node = ( kd_node * ) malloc( sizeof( kd_node ) );

    if ( node == NULL )
        return NULL;

    node->point = ( int * ) malloc( sizeof( int ) * k );

    if ( node->point == NULL)
    {
        free( node );
        return NULL;
    }

    for ( int i = 0; i < k; i++ ) node->point[ i ] = point[ i ];

    node->l = NULL;
    node->r = NULL;
    node->item = item;

    return node;
}

static int kd_insert_util( kd_node **node, int k, int point[], void *item, int depth )
{
    if ( *node == NULL )
    {
        *node = new_kd_node( k, point, item );
        return 1;
    }

    // don't insert if identical ( *node ) exists
    if ( ptcmp( k, point, ( *node )->point ) )
    {
        return 0;
    }

    int axis = depth % k;

    if ( point[ axis ] >= ( *node )->point[ axis ] )
    {
        return kd_insert_util( &( *node )->r, k, point, item, depth + 1 );
    }
    else
    {
        return kd_insert_util( &( *node )->l, k, point, item, depth + 1 );
    }
}

int kd_insert( kd_tree *tree, int point[], void *item )
{
    if ( tree == NULL )
        return -1;

    int status = kd_insert_util( &tree->root, tree->k, point, item, 0 );
    tree->size += status;
    return status;
}

static void swap_node( kd_node *des, kd_node *src, int k )
{
    void *item = des->item;

    des->item = src->item;
    src->item = item;

    for ( int i = 0; i < k; i++ )
        des->point[ i ] = src->point[ i ];
}

static kd_node *min_node( kd_node *x, kd_node *y, kd_node *z, int axis )
{
    kd_node *res = x;
    if ( y != NULL && y->point[ axis ] < res->point[ axis ] )
        res = y;
    if ( z != NULL && z->point[ axis ] < res->point[ axis ] )
        res = z;
    return res;
}

static kd_node *find_min( kd_node *node, int k, int axis, int depth )
{
    if ( node == NULL )
        return NULL;

    int cur_axis = depth % k;

    if ( cur_axis == axis )
    {
        if ( node->l == NULL )
            return node;
        return find_min( node->l, k, axis, depth + 1 );
    }

    return min_node(
            node,
            find_min( node->l, k, axis, depth + 1 ),
            find_min( node->r, k, axis, depth + 1 ),
            axis );
}

static int kd_remove_util( kd_node **node, int k, int point[], int depth )
{
    if ( ( *node ) == NULL )
        return 0;

    int axis = depth % k;

    if ( ptcmp( k, point, ( *node )->point ) )
    {
        if ( ( *node )->r != NULL )
        {
            kd_node *min = find_min( ( *node )->r, k, axis, depth + 1 );
            swap_node( *node, min, k );
            return kd_remove_util( &( *node )->r, k, min->point, depth + 1 );
        }
        if ( ( *node )->l != NULL )
        {
            kd_node *min = find_min( ( *node )->l, k, axis, depth + 1 );
            swap_node( *node, min, k );

            int status = kd_remove_util( &( *node )->l, k, min->point, depth + 1 );

            ( *node )->r = ( *node )->l;
            ( *node )->l = NULL;

            return status;
        }
        else
        {
            free_node( *node );
            *node = NULL;
        }
    }
    else
    {
        if ( point[ axis ] >= ( *node )->point[ axis ] )
        {
            return kd_remove_util( &( *node )->r, k, point, depth + 1 );
        }
        else
        {
            return kd_remove_util( &( *node )->l, k, point, depth + 1 );
        }
    }

    return 1;
}

int kd_remove( kd_tree *tree, int point[] )
{
    if ( tree == NULL )
        return -1;

    int status = kd_remove_util( &tree->root, tree->k, point, 0 );
    tree->size -= status;
    return status;
}

static kd_node *kd_search_util( kd_node *node, int k, int point[], int depth )
{
    if ( node == NULL )
    {
        return NULL;
    }

    // found node
    if ( ptcmp( k, point, node->point ) )
    {
        return node;
    }

    int axis = depth % k;

    if ( point[ axis ] >= node->point[ axis ] )
    {
        return kd_search_util( node->r, k, point, depth + 1 );
    }
    else
    {
        return kd_search_util( node->l, k, point, depth + 1 );
    }
}

void *kd_search( kd_tree *tree, int point[] )
{
    if ( tree == NULL )
        return NULL;

    return kd_search_util( tree->root, tree->k, point, 0 )->item;
}

static void free_node( kd_node *node )
{
    free( node->point );
    free( node );
}

static void kd_free_util( kd_node *node, void ( *free_item )( void *item ) )
{
    if ( node == NULL )
        return;

    kd_free_util( node->r, free_item );
    kd_free_util( node->l, free_item );

    free_item( node->item );
    free_node( node );
}

void kd_free( kd_tree *tree )
{
    if ( tree->root == NULL )
        return;

    kd_free_util( tree->root, tree->free_item );
}
