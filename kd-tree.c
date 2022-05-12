#include <stdlib.h>
#include "kd-tree.h"

#define PI 3.1415926535897932384626433832795f

typedef struct kd_node kd_node;

struct kd_node
{
    KD_DATA_TYPE *point;

    kd_node *r;
    kd_node *l;

    void *item;
};

struct kd_tree
{
    kd_node *root;

    int size;
    int k;

    void ( *free_item )( void * );
};

// check if two points are the same
static int ptcmp( KD_DATA_TYPE pt_a[], KD_DATA_TYPE pt_b[], int k )
{
    for ( int i = 0; i < k; i++ )
        if ( pt_a[ i ] != pt_b[ i ] )
            return 0;

    return 1;
}

/*
 * new kd tree
 */

kd_tree *new_kd_tree( int k, void ( *free_item )( void * ) )
{
    // sanity check
    if ( k < 2 )
        return NULL;

    kd_tree *tree = ( kd_tree * ) malloc( sizeof( kd_tree ) );

    if ( tree == NULL )
        return NULL;

    // init
    tree->root = NULL;
    tree->size = 0;
    tree->k = k;
    tree->free_item = free_item;

    return tree;
}

static kd_node *new_kd_node( int k, KD_DATA_TYPE point[], void *item )
{
    if ( k < 2 )
        return NULL;

    kd_node *node = ( kd_node * ) malloc( sizeof( kd_node ) );

    if ( node == NULL )
        return NULL;

    node->point = ( KD_DATA_TYPE * ) malloc( sizeof( KD_DATA_TYPE ) * k );

    if ( node->point == NULL)
    {
        free( node );
        return NULL;
    }

    for ( int i = 0; i < k; i++ )
        node->point[ i ] = point[ i ];

    node->r = NULL;
    node->l = NULL;
    node->item = item;

    return node;
}

/*
 * getter functions
 */

int kd_size( kd_tree *tree )
{
    return tree->size;
}

int kd_dim( kd_tree *tree )
{
    return tree->k;
}

/*
 * insert remove
 */

static kd_node *kd_replace_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], void *item, int depth, void **result )
{
    int k = tree->k;
    int axis = depth % k;

    if ( node == NULL )
    {
        node = new_kd_node( k, point, item );
        *result = item;
        tree->size++;
    }
    else if ( ptcmp( point, node->point, k ) )
    {
        // swap items
        *result = node->item;
        node->item = item;
    }
    else
    {
        // go right or left depending on depth and point
        if ( point[ axis ] >= node->point[ axis ] )
        {
            node->r = kd_replace_util( tree, node->r, point, item, depth + 1, result );
        }
        else
        {
            node->l = kd_replace_util( tree, node->l, point, item, depth + 1, result );
        }
    }

    return node;
}

void *kd_replace( kd_tree *tree, KD_DATA_TYPE point[], void *item )
{
    if ( tree == NULL )
        return NULL;

    void *result = NULL;
    tree->root = kd_replace_util( tree, tree->root, point, item, 0, &result );
    return result;
}

static kd_node *kd_insert_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], void *item, int depth, void **result )
{
    int k = tree->k;
    int axis = depth % k;

    if ( node == NULL )
    {
        node = new_kd_node( k, point, item );
        *result = item;
        tree->size++;
    }
    else if ( ptcmp( point, node->point, k ) )
    {
        *result = node->item;
    }
    else
    {
        // go right or left depending on depth and point
        if ( point[ axis ] >= node->point[ axis ] )
        {
            node->r = kd_insert_util( tree, node->r, point, item, depth + 1, result );
        }
        else
        {
            node->l = kd_insert_util( tree, node->l, point, item, depth + 1, result );
        }
    }

    return node;
}

void *kd_insert( kd_tree *tree, KD_DATA_TYPE point[], void *item )
{
    if ( tree == NULL )
        return NULL;

    void *result = NULL;
    tree->root = kd_insert_util( tree, tree->root, point, item, 0, &result );
    return result;
}

// swap item in node. copy point dst to src
static void swap_and_copy( kd_node *dst, kd_node *src, int k )
{
    void *item = dst->item;

    dst->item = src->item;
    src->item = item;

    for ( int i = 0; i < k; i++ )
        dst->point[ i ] = src->point[ i ];
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

static kd_node *kd_delete_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], int depth )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;
    int axis = depth % k;

    if ( ptcmp( point, node->point, k ) )
    {
        if ( node->r != NULL )
        {
            kd_node *min = find_min( node->r, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kd_delete_util( tree, node->r, node->point, depth + 1 );
        }
        else if ( node->l != NULL )
        {
            kd_node *min = find_min( node->l, k, axis, depth + 1 );
            swap_and_copy( node, min, k );

            // when right is null we need to move left item to the right
            node->r = kd_delete_util( tree, node->l, node->point, depth + 1 );
            node->l = NULL;
        }
        else
        {
            if ( tree->free_item )
                tree->free_item( node->item );

            free( node->point );
            free( node );
            node = NULL;
            tree->size--;
        }
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            node->r = kd_delete_util( tree, node->r, point, depth + 1 );
        }
        else
        {
            node->l = kd_delete_util( tree, node->l, point, depth + 1 );
        }
    }

    return node;
}

int kd_delete( kd_tree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return -1;

    int size = tree->size;
    tree->root = kd_delete_util( tree, tree->root, point, 0 );
    return ( size != tree->size );
}

static kd_node *kd_remove_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], int depth, void **result )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;
    int axis = depth % k;

    if ( ptcmp( point, node->point, k ) )
    {
        if ( node->r != NULL )
        {
            kd_node *min = find_min( node->r, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kd_remove_util( tree, node->r, node->point, depth + 1, result );
        }
        else if ( node->l != NULL )
        {
            kd_node *min = find_min( node->l, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kd_remove_util( tree, node->l, node->point, depth + 1, result );
            node->l = NULL;
        }
        else
        {
            // get result then free node
            *result = node->item;
            free( node->point );
            free( node );
            node = NULL;
            tree->size--;
        }
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            node->r = kd_remove_util( tree, node->r, point, depth + 1, result );
        }
        else
        {
            node->l = kd_remove_util( tree, node->l, point, depth + 1, result );
        }
    }

    return node;
}

// pull just removes the node from tree and returns the item at that node
void *kd_remove( kd_tree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return NULL;

    void *item = NULL;
    tree->root = kd_remove_util( tree, tree->root, point, 0, &item );
    return item;
}

/*
 * query
 */

static int intersects_range( KD_DATA_TYPE pt_a[], KD_DATA_TYPE pt_b[], KD_DATA_TYPE range, int axis )
{
    return ( ( pt_a[ axis ] <= ( pt_b[ axis ] + range ) ) && ( pt_a[ axis ] >= ( pt_b[ axis ] - range ) ) );
}

static int overlaps_range( KD_DATA_TYPE pt_a[], KD_DATA_TYPE pt_b[], KD_DATA_TYPE range, int k )
{
    KD_DATA_TYPE distance = 0;
    range = range * range;

    for ( int i = 0; i < k; i++ )
    {
        KD_DATA_TYPE a = pt_a[ i ] - pt_b[ i ];
        distance += a * a;
    }

    return ( distance <= range );
}

static int kd_query_range_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], KD_DATA_TYPE range, int depth, void ***query )
{
    if ( node == NULL )
        return 0;

    int k = tree->k;
    int axis = depth % k;
    int result = 0;

    if ( overlaps_range( node->point, point, range, k ) )
    {
        // add to query if overlaps
        *( ( *query )++ ) = node->item;

        // if it overlaps that means more points could be on the right and/or left
        result += kd_query_range_util( tree, node->l, point, range, depth + 1, query );
        result += kd_query_range_util( tree, node->r, point, range, depth + 1, query );
        result++;
    }
    else if ( intersects_range( node->point, point, range, axis ) )
    {
        // if the point intersects at the correct axis then more points could be on the right and/or left
        result += kd_query_range_util( tree, node->l, point, range, depth + 1, query );
        result += kd_query_range_util( tree, node->r, point, range, depth + 1, query );
    }
    else
    {
        // nothing was found keep looking
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kd_query_range_util( tree, node->r, point, range, depth + 1, query );
        }
        else
        {
            result += kd_query_range_util( tree, node->l, point, range, depth + 1, query );
        }
    }

    return result;
}

void **kd_query_range( kd_tree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE range, int *length )
{
    if ( tree == NULL )
        return NULL;

    void **query = ( void ** ) malloc( sizeof( void * ) * PI * range * range + 2 );
    void **head = query;
    int l = kd_query_range_util( tree, tree->root, point, range, 0, &query );

    if ( l > 0 )
    {
        head = ( void ** ) realloc( head, sizeof( void * ) * l );
    }
    else
    {
        free( head );
        head = NULL;
    }

    if ( length != NULL )
        *length = l;

    return head;
}

static int intersects_dim( KD_DATA_TYPE pt_a[], KD_DATA_TYPE pt_b[], KD_DATA_TYPE dim[], int axis )
{
    return ( ( pt_a[ axis ] >= pt_b[ axis ] ) && ( pt_a[ axis ] <= ( pt_b[ axis ] + dim[ axis ] ) ) );
}

static int overlaps_dim( KD_DATA_TYPE pt_a[], KD_DATA_TYPE pt_b[], KD_DATA_TYPE dim[], int k )
{
    for ( int i = 0; i < k; i++ )
        if ( ( pt_a[ i ] < pt_b[ i ] ) || ( pt_a[ i ] >= ( pt_b[ i ] + dim[ i ] ) ) )
            return 0;

    return 1;
}

static int kd_query_dim_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], KD_DATA_TYPE dim[], int depth, void ***query )
{
    if ( node == NULL )
        return 0;

    int k = tree->k;
    int axis = depth % k;
    int result = 0;

    if ( overlaps_dim( node->point, point, dim, k ) )
    {
        *( ( *query )++ ) = node->item;
        result += kd_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        result += kd_query_dim_util( tree, node->r, point, dim, depth + 1, query );
        result++;
    }
    else if ( intersects_dim( node->point, point, dim, axis ) )
    {
        result += kd_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        result += kd_query_dim_util( tree, node->r, point, dim, depth + 1, query );
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kd_query_dim_util( tree, node->r, point, dim, depth + 1, query );
        }
        else
        {
            result += kd_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        }
    }

    return result;
}

void **kd_query_dim( kd_tree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE dim[], int *length )
{
    if ( tree == NULL )
        return NULL;

    KD_DATA_TYPE area = 1;

    for ( int i = 0; i < tree->k; i++ )
        area *= dim[ i ];

    void **query = ( void ** ) malloc( sizeof( void * ) * area + 1 );
    void **head = query;
    int l = kd_query_dim_util( tree, tree->root, point, dim, 0, &query );

    if ( l > 0 )
    {
        head = ( void ** ) realloc( head, sizeof( void * ) * l );
    }
    else
    {
        free( head );
        head = NULL;
    }

    if ( length != NULL )
        *length = l;

    return head;
}

/*
 * search
 */

static kd_node *kd_search_util( kd_tree *tree, kd_node *node, KD_DATA_TYPE point[], int depth )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;

    if ( ptcmp( point, node->point, k ) )
        return node;

    int axis = depth % k;

    if ( point[ axis ] >= node->point[ axis ] )
    {
        return kd_search_util( tree, node->r, point, depth + 1 );
    }
    else
    {
        return kd_search_util( tree, node->l, point, depth + 1 );
    }
}

void *kd_search( kd_tree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return NULL;

    kd_node *res = kd_search_util( tree, tree->root, point, 0 );
    return res == NULL ? NULL : res->item;
}

/*
 * free memory
 */

static void kd_free_util( kd_tree *tree, kd_node *node )
{
    if ( node == NULL )
        return;

    kd_free_util( tree, node->r );
    kd_free_util( tree, node->l );

    if ( tree->free_item )
        tree->free_item( node->item );

    free( node->point );
    free( node );
}

void kd_free( kd_tree *tree )
{
    if ( tree == NULL )
        return;

    kd_free_util( tree, tree->root );
    free( tree );
}
