#include <stdlib.h>
#include "kdtree.h"

#define PI 3.1415926535897932384626433832795f

typedef struct kdt_node kdt_node;

struct kdt_node
{
    KD_DATA_TYPE *point;

    kdt_node *r;
    kdt_node *l;

    void *item;
};

struct kdtree
{
    kdt_node *root;

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

kdtree *new_kdtree( int k, void ( *free_item )( void * ) )
{
    // sanity check
    if ( k < 2 )
        return NULL;

    kdtree *tree = ( kdtree * ) malloc( sizeof( kdtree ) );

    if ( tree == NULL )
        return NULL;

    // init
    tree->root = NULL;
    tree->size = 0;
    tree->k = k;
    tree->free_item = free_item;

    return tree;
}

/*
 * free memory
 */

static void kdt_free_util( kdtree *tree, kdt_node *node )
{
    if ( node == NULL )
        return;

    kdt_free_util( tree, node->r );
    kdt_free_util( tree, node->l );

    if ( tree->free_item )
        tree->free_item( node->item );

    free( node->point );
    free( node );
}

void free_kdtree( kdtree *tree )
{
    if ( tree == NULL )
        return;

    kdt_free_util( tree, tree->root );
    free( tree );
}

/*
 * new node
 */

static kdt_node *new_kdt_node( int k, KD_DATA_TYPE point[], void *item )
{
    if ( k < 2 )
        return NULL;

    kdt_node *node = ( kdt_node * ) malloc( sizeof( kdt_node ) );

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

int kdt_size( kdtree *tree )
{
    return tree->size;
}

int kdt_dim( kdtree *tree )
{
    return tree->k;
}

/*
 * insert remove
 */

static kdt_node *kdt_replace_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], void *item, int depth, void **result )
{
    int k = tree->k;
    int axis = depth % k;

    if ( node == NULL )
    {
        node = new_kdt_node( k, point, item );
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
            node->r = kdt_replace_util( tree, node->r, point, item, depth + 1, result );
        }
        else
        {
            node->l = kdt_replace_util( tree, node->l, point, item, depth + 1, result );
        }
    }

    return node;
}

void *kdt_replace( kdtree *tree, KD_DATA_TYPE point[], void *item )
{
    if ( tree == NULL )
        return NULL;

    void *result = NULL;
    tree->root = kdt_replace_util( tree, tree->root, point, item, 0, &result );
    return result;
}

static kdt_node *kdt_insert_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], void *item, int depth, void **result )
{
    int k = tree->k;
    int axis = depth % k;

    if ( node == NULL )
    {
        node = new_kdt_node( k, point, item );
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
            node->r = kdt_insert_util( tree, node->r, point, item, depth + 1, result );
        }
        else
        {
            node->l = kdt_insert_util( tree, node->l, point, item, depth + 1, result );
        }
    }

    return node;
}

void *kdt_insert( kdtree *tree, KD_DATA_TYPE point[], void *item )
{
    if ( tree == NULL )
        return NULL;

    void *result = NULL;
    tree->root = kdt_insert_util( tree, tree->root, point, item, 0, &result );
    return result;
}

// swap item in node. copy point dst to src
static void swap_and_copy( kdt_node *dst, kdt_node *src, int k )
{
    void *item = dst->item;

    dst->item = src->item;
    src->item = item;

    for ( int i = 0; i < k; i++ )
        dst->point[ i ] = src->point[ i ];
}

static kdt_node *min_node( kdt_node *x, kdt_node *y, kdt_node *z, int axis )
{
    kdt_node *res = x;
    if ( y != NULL && y->point[ axis ] < res->point[ axis ] )
        res = y;
    if ( z != NULL && z->point[ axis ] < res->point[ axis ] )
        res = z;
    return res;
}

static kdt_node *find_min( kdt_node *node, int k, int axis, int depth )
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

static kdt_node *kdt_delete_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], int depth )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;
    int axis = depth % k;

    if ( ptcmp( point, node->point, k ) )
    {
        if ( node->r != NULL )
        {
            kdt_node *min = find_min( node->r, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kdt_delete_util( tree, node->r, node->point, depth + 1 );
        }
        else if ( node->l != NULL )
        {
            kdt_node *min = find_min( node->l, k, axis, depth + 1 );
            swap_and_copy( node, min, k );

            // when right is null we need to move left item to the right
            node->r = kdt_delete_util( tree, node->l, node->point, depth + 1 );
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
            node->r = kdt_delete_util( tree, node->r, point, depth + 1 );
        }
        else
        {
            node->l = kdt_delete_util( tree, node->l, point, depth + 1 );
        }
    }

    return node;
}

int kdt_delete( kdtree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return -1;

    int size = tree->size;
    tree->root = kdt_delete_util( tree, tree->root, point, 0 );
    return ( size != tree->size );
}

static kdt_node *kdt_remove_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], int depth, void **result )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;
    int axis = depth % k;

    if ( ptcmp( point, node->point, k ) )
    {
        if ( node->r != NULL )
        {
            kdt_node *min = find_min( node->r, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kdt_remove_util( tree, node->r, node->point, depth + 1, result );
        }
        else if ( node->l != NULL )
        {
            kdt_node *min = find_min( node->l, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kdt_remove_util( tree, node->l, node->point, depth + 1, result );
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
            node->r = kdt_remove_util( tree, node->r, point, depth + 1, result );
        }
        else
        {
            node->l = kdt_remove_util( tree, node->l, point, depth + 1, result );
        }
    }

    return node;
}

// pull just removes the node from tree and returns the item at that node
void *kdt_remove( kdtree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return NULL;

    void *item = NULL;
    tree->root = kdt_remove_util( tree, tree->root, point, 0, &item );
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

static int kdt_query_range_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], KD_DATA_TYPE range, int depth, void ***query )
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
        result += kdt_query_range_util( tree, node->l, point, range, depth + 1, query );
        result += kdt_query_range_util( tree, node->r, point, range, depth + 1, query );
        result++;
    }
    else if ( intersects_range( node->point, point, range, axis ) )
    {
        // if the point intersects at the correct axis then more points could be on the right and/or left
        result += kdt_query_range_util( tree, node->l, point, range, depth + 1, query );
        result += kdt_query_range_util( tree, node->r, point, range, depth + 1, query );
    }
    else
    {
        // nothing was found keep looking
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kdt_query_range_util( tree, node->r, point, range, depth + 1, query );
        }
        else
        {
            result += kdt_query_range_util( tree, node->l, point, range, depth + 1, query );
        }
    }

    return result;
}

void **kdt_query_range( kdtree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE range, int *length )
{
    if ( tree == NULL )
        return NULL;

    void **query = ( void ** ) malloc( sizeof( void * ) * PI * range * range + 2 );
    void **head = query;
    int l = kdt_query_range_util( tree, tree->root, point, range, 0, &query );

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

static int kdt_query_dim_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], KD_DATA_TYPE dim[], int depth, void ***query )
{
    if ( node == NULL )
        return 0;

    int k = tree->k;
    int axis = depth % k;
    int result = 0;

    if ( overlaps_dim( node->point, point, dim, k ) )
    {
        *( ( *query )++ ) = node->item;
        result += kdt_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        result += kdt_query_dim_util( tree, node->r, point, dim, depth + 1, query );
        result++;
    }
    else if ( intersects_dim( node->point, point, dim, axis ) )
    {
        result += kdt_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        result += kdt_query_dim_util( tree, node->r, point, dim, depth + 1, query );
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kdt_query_dim_util( tree, node->r, point, dim, depth + 1, query );
        }
        else
        {
            result += kdt_query_dim_util( tree, node->l, point, dim, depth + 1, query );
        }
    }

    return result;
}

void **kdt_query_dim( kdtree *tree, KD_DATA_TYPE point[], KD_DATA_TYPE dim[], int *length )
{
    if ( tree == NULL )
        return NULL;

    KD_DATA_TYPE area = 1;

    for ( int i = 0; i < tree->k; i++ )
        area *= dim[ i ];

    void **query = ( void ** ) malloc( sizeof( void * ) * area + 1 );
    void **head = query;
    int l = kdt_query_dim_util( tree, tree->root, point, dim, 0, &query );

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

static kdt_node *kdt_search_util( kdtree *tree, kdt_node *node, KD_DATA_TYPE point[], int depth )
{
    if ( node == NULL )
        return NULL;

    int k = tree->k;

    if ( ptcmp( point, node->point, k ) )
        return node;

    int axis = depth % k;

    if ( point[ axis ] >= node->point[ axis ] )
    {
        return kdt_search_util( tree, node->r, point, depth + 1 );
    }
    else
    {
        return kdt_search_util( tree, node->l, point, depth + 1 );
    }
}

void *kdt_search( kdtree *tree, KD_DATA_TYPE point[] )
{
    if ( tree == NULL )
        return NULL;

    kdt_node *res = kdt_search_util( tree, tree->root, point, 0 );
    return res == NULL ? NULL : res->item;
}
