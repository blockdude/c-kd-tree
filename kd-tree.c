#include <stdlib.h>
#include "kd-tree.h"

#define PI 3.1415926535897932384626433832795f

struct kd_node
{
    int *point;

    // used for kd tree
    kd_node *r;
    kd_node *l;

    // used for linked list
    kd_node *n;

    void *item;
};

struct kd_tree
{
    kd_node *root;

    int size;
    int k;

    void ( *free_item )( void * );
};

struct kd_result
{
    kd_node *head;
    int length;
};

static int ptcmp( int pt_a[], int pt_b[], int k )
{
    for ( int i = 0; i < k; i++ )
        if ( pt_a[ i ] != pt_b[ i ] )
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

    node->r = NULL;
    node->l = NULL;
    node->n = NULL;
    node->item = item;

    return node;
}

int kd_size( kd_tree *tree )
{
    return tree->size;
}

int kd_dim( kd_tree *tree )
{
    return tree->k;
}

static kd_node *kd_insert_util( kd_tree *tree, kd_node *node, int point[], void *item, int depth )
{
    int k = tree->k;
    int axis = depth % k;

    if ( node == NULL )
    {
        node = new_kd_node( k, point, item );
        tree->size++;
    }
    else if ( !ptcmp( point, node->point, k ) )
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            node->r = kd_insert_util( tree, node->r, point, item, depth + 1 );
        }
        else
        {
            node->l = kd_insert_util( tree, node->l, point, item, depth + 1 );
        }
    }

    return node;
}

int kd_insert( kd_tree *tree, int point[], void *item )
{
    if ( tree == NULL )
        return -1;

    int size = tree->size;
    tree->root = kd_insert_util( tree, tree->root, point, item, 0 );
    return ( size != tree->size );
}

static void swap_and_copy( kd_node *des, kd_node *src, int k )
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

static kd_node *kd_remove_util( kd_tree *tree, kd_node *node, int point[], int depth )
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
            node->r = kd_remove_util( tree, node->r, node->point, depth + 1 );
        }
        else if ( node->l != NULL )
        {
            kd_node *min = find_min( node->l, k, axis, depth + 1 );
            swap_and_copy( node, min, k );
            node->r = kd_remove_util( tree, node->l, node->point, depth + 1 );
            node->l = NULL;
        }
        else
        {
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
            node->r = kd_remove_util( tree, node->r, point, depth + 1 );
        }
        else
        {
            node->l = kd_remove_util( tree, node->l, point, depth + 1 );
        }
    }

    return node;
}

int kd_remove( kd_tree *tree, int point[] )
{
    if ( tree == NULL )
        return -1;

    int size = tree->size;
    tree->root = kd_remove_util( tree, tree->root, point, 0 );
    return ( size != tree->size );
}

static void **query;

static int intersects_range( int pt_a[], int pt_b[], int range, int axis )
{
    return ( ( pt_a[ axis ] <= ( pt_b[ axis ] + range ) ) && ( pt_a[ axis ] >= ( pt_b[ axis ] - range ) ) );
}

static int overlaps_range( int pt_a[], int pt_b[], int range, int k )
{
    int distance = 0;
    range = range * range;

    for ( int i = 0; i < k; i++ )
    {
        int a = pt_a[ i ] - pt_b[ i ];
        distance += a * a;
    }

    return ( distance <= range );
}

static int kd_query_range_util( kd_tree *tree, kd_node *node, int point[], int range, int depth )
{
    if ( node == NULL )
        return 0;

    int k = tree->k;
    int axis = depth % k;
    int result = 0;

    if ( overlaps_range( node->point, point, range, k ) )
    {
        *( query++ ) = node->item;
        result += kd_query_range_util( tree, node->l, point, range, depth + 1 );
        result += kd_query_range_util( tree, node->r, point, range, depth + 1 );
        result++;
    }
    else if ( intersects_range( node->point, point, range, axis ) )
    {
        result += kd_query_range_util( tree, node->l, point, range, depth + 1 );
        result += kd_query_range_util( tree, node->r, point, range, depth + 1 );
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kd_query_range_util( tree, node->r, point, range, depth + 1 );
        }
        else
        {
            result += kd_query_range_util( tree, node->l, point, range, depth + 1 );
        }
    }

    return result;
}

void **kd_query_range( kd_tree *tree, int point[], int range, int *length )
{
    if ( tree == NULL )
        return NULL;

    query = ( void ** ) malloc( sizeof( void * ) * PI * range * range + 2 );
    void **head = query;
    *length = kd_query_range_util( tree, tree->root, point, range, 0 );
    return head;
}

static int intersects_dim( int pt_a[], int pt_b[], int dim[], int axis )
{
    return ( ( pt_a[ axis ] <= pt_b[ axis ] ) && ( pt_a[ axis ] >= ( pt_b[ axis ] - dim[ axis ] ) ) );
}

static int overlaps_dim( int pt_a[], int pt_b[], int dim[], int k )
{
    for ( int i = 0; i < k; i++ )
        if ( ( pt_a[ i ] > pt_b[ i ] ) || ( pt_a[ i ] < ( pt_b[ i ] - dim[ i ] ) ) )
            return 0;

    return 1;
}

static int kd_query_dim_util( kd_tree *tree, kd_node *node, int point[], int dim[], int depth )
{
    if ( node == NULL )
        return 0;

    int k = tree->k;
    int axis = depth % k;
    int result = 0;

    if ( overlaps_dim( node->point, point, dim, k ) )
    {
        *( query++ ) = node->item;
        result += kd_query_dim_util( tree, node->l, point, dim, depth + 1 );
        result += kd_query_dim_util( tree, node->r, point, dim, depth + 1 );
        result++;
    }
    else if ( intersects_dim( node->point, point, dim, axis ) )
    {
        result += kd_query_dim_util( tree, node->l, point, dim, depth + 1 );
        result += kd_query_dim_util( tree, node->r, point, dim, depth + 1 );
    }
    else
    {
        if ( point[ axis ] >= node->point[ axis ] )
        {
            result += kd_query_dim_util( tree, node->r, point, dim, depth + 1 );
        }
        else
        {
            result += kd_query_dim_util( tree, node->l, point, dim, depth + 1 );
        }
    }
    
    return result;
}

void **kd_query_dim( kd_tree *tree, int point[], int dim[], int *length )
{
    if ( tree == NULL )
        return NULL;

    int area = 1;

    for ( int i = 0; i < tree->k; i++ )
        area *= dim[ i ];

    query = ( void ** ) malloc( sizeof( void * ) * area + 1 );
    void **head = query;
    *length = kd_query_dim_util( tree, tree->root, point, dim, 0 );
    return head;
}

static kd_node *kd_search_util( kd_tree *tree, kd_node *node, int point[], int depth )
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

void *kd_search( kd_tree *tree, int point[] )
{
    if ( tree == NULL )
        return NULL;

    return kd_search_util( tree, tree->root, point, 0 )->item;
}

static void kd_free_util( kd_tree *tree, kd_node *node )
{
    if ( node == NULL )
        return;

    kd_free_util( tree, node->r );
    kd_free_util( tree, node->l );

    tree->free_item( node->item );
    free( node->point );
    free( node );
}

void kd_free( kd_tree *tree )
{
    if ( tree->root == NULL )
        return;

    kd_free_util( tree, tree->root );
}
