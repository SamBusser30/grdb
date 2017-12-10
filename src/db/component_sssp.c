#include "tuple.h"
#include <cli.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "config.h"
#include "cli.h"
#include "graph.h"
#define INT_type 4 //int is 4th in enum
#define inf INT_MAX
#define min(x, y) (((x) < (y)) ? (x) : (y))  //defining a min function for dijkstra


int find_weight(component_t c, vertexid_t v1, vertexid_t v2, char* attr_name){
    struct edge e1;
    edge_t e2;
    int offset, weight;
    
    edge_init(&e1);
    edge_set_vertices(&e1, v1, v2); //function gotten from Frank's other code
    
    e2 = component_find_edge_by_ids(c, &e1); //another function from Frank
    
    if(e2 == NULL){
        return inf;
    }
    
    offset = tuple_get_offset(e2->tuple, attr_name); //another from Frank
    weight = tuple_get_int(e2->tuple->buf + offset);  //ditto
    return weight;
}




// int find_weight(vertexid_t v1, vertexid_t v2, component_t c){

// 	off_t off;
// 	ssize_t len, size;
// 	vertexid_t id1, id2;
// 	struct tuple tuple;
// 	char *buf;
// 	int readlen;
// 	int edge_weight, offset;
// 	char s[BUFSIZE];
// 	attribute_t attr;

// 	bool found_edge = false;

// 	//allocating space for the path to go into e
// 	memset(s, 0, BUFSIZE);
// 	//setting the path to s
// 	sprintf(s, "%s/%d/%d/e", grdbdir, gno, cno);
// 	//opens file found at that path
// 	//efd is the edge file descriptor
// 	c->efd = open(s, O_RDONLY);

// 	if (c->se == NULL)
// 		size = 0;
// 	else
// 		size = schema_size(c->se);

// 	readlen = (sizeof(vertexid_t) << 1) + size;
// 	free(buf);
// 	buf = malloc(readlen);

// 	off = 0;

// 	while(found_edge == false){

// 		lseek(c->efd, off, SEEK_SET);
// 		len = read(c->efd, buf, readlen);
// 		if (len <= 0)
// 			break;

// 		id1 = *((vertexid_t *) buf);
// 		id2 = *((vertexid_t *) (buf + sizeof(vertexid_t)));

// 		if(id1 == v1 && id2 == v2){
// 			//get edge attribute of these 
// 			printf("Tuples found: ");
// 			printf("(%llu,%llu)\n", id1, id2);

// 			memset(&tuple, 0, sizeof(struct tuple));
// 			tuple.s = c->se;
// 			tuple.len = size;
// 			tuple.buf = buf + (sizeof(vertexid_t)<<1);

// 			for (attr = tuple.s->attrlist; attr != NULL; attr = attr->next) {
// 				offset = tuple_get_offset(&tuple, attr->name);
// 				if(offset >= 0 && attr->bt == INTEGER){
// 					edge_weight = tuple_get_int(tuple.buf + offset);
// 					printf("The edge weight is: %d\n",edge_weight);
// 					return edge_weight;
// 				}
// 			}	
// 			found_edge = true;
// 		}
// 		off += readlen;
// 	}

// 	return -1;
// }

int get_id_index(vertexid_t id, vertexid_t* list,int count){
    
	//loop through list, find id, return index
    for(int i = 0; i < count; i++){
        if(list[i] == id){
            return i;
        }
    }
    return -1;
}

/* find shortes path between vertex 1 and any other vertex in component c */
int component_sssp(component_t c, vertexid_t v1, vertexid_t v2, int *n, int *total_weight, vertexid_t **path){

    //As told in class, assume start vertex is 1
    if(v1 != 1){
        printf("TStarting vertex needs to be 1\n");
        return -1;
    }
    
    int vertices;
    
    //arrays to keep track of elements, will be size of the variables
    vertexid_t *s;
    vertexid_t *sssp_list;
    int s_count;
    int sssp_list_count;
    
    //initializing size arrays for dijkstra
    vertexid_t *v_list;
    int *c_list;
    vertexid_t *p_list;
    
    //adding file descriptors to c
    c->efd = edge_file_init(gno, cno);
    c->vfd = vertex_file_init(gno, cno);
    
    //need to check schema to make sure there is an int 
    attribute_t weight_name;
    attribute_t attr;
    
    for(attr = c->se->attrlist; attr != NULL; attr = attr->next){
        if(attr->bt == INT_type)
            weight_name = attr;
    }
    if(!weight_name){
        printf("No integer attributes found to use for weight\n");
        return -1;
    }
    
    //need to find the max size for our arrays, and then malloc them -> find # of vertices
    ssize_t size, length;
    char* buffer;
    off_t os; 
    int readlen, count;
    if(c->sv == NULL)	//need size of our vertex schema to set buffer
    	size = 0;
    else
    	size = schema_size(c->sv);

    readlen = sizeof(vertexid_t) + size; //this will be the total size of the buffer we need 
    buffer = malloc(readlen);

    count = 0; 
    //count all the vertices 
    for (os=0;; os += readlen){
    	//printf("Starting infinite loop 3");
    	lseek(c->vfd, os, SEEK_SET);
    	length  = read(c->vfd, buffer, readlen);
    	
    	if (length <= 0 )
    		break;
    	(count)++;
    }
    vertices = count;
    free(buffer);
    
    //now that we have number of vertices, we can properly init arrays
    p_list = malloc(vertices * sizeof(vertexid_t));
    v_list = malloc(vertices * sizeof(vertexid_t));
    c_list = malloc(vertices * sizeof(int));
    sssp_list = malloc(vertices * sizeof(int));

    //now need to get vertices, very similar to code for counting vertices
    ssize_t size2, len2;
    int readlen2;
    char* buf2;
    off_t offset2;
    vertexid_t v;

    if(c->sv == NULL)
    	size2 = 0;
    else 
    	size2 = schema_size(c->sv);

    readlen2 = sizeof(vertexid_t) + size2;
    buf2 = malloc(readlen2);

    for(int i = 0,offset2 = 0;; i++, offset2 += readlen2){
    	//printf("Starting infinite loop 5")
    	lseek(c->vfd, offset2, SEEK_SET);
    	len2 = read(c->vfd, buf2, readlen2);
    	if(len2 <= 0)
    		break;

    	v = *((vertexid_t *)buf2);
    	v_list[i] = v;
    }
    free(buf2);

    for(int i = 0; i < vertices; i++){
        c_list[i] = inf;
        p_list[i] = inf;
    }
    
    //define traversal variables
    int start;
    int end;
    int min_index;
    int min;
    int in_vertex_schema;
    vertexid_t dij_w, dij_v;
    
    //check if start and end vertices in component, get them
    start = -1;
    end = -1;
    for(int k = 0; k < vertices; k++){
        //printf("Starting loop 6");
        if(v_list[k] == v1)
            start = k;
        if(v_list[k] == v2)
            end = k;
    }
    if(start == -1 || end == -1){
        printf("Could not find start or end vertex in graph\n");
        return -1;
    }
    
    //setting initial edge weights
    for(int i = 1; i < vertices; i++){
        //printf("Starting loop7");
        int tmp_wt = find_weight(c, v1, v_list[i], weight_name->name);
        if(tmp_wt != inf){
            p_list[i] = v1;
        }
        else if(tmp_wt <= 0){
            printf("found zero or negative weight on edge\n");
            return -1;
        }
    }
    
    //start traversing component now 
    for(int i = 1; i < vertices; i++){
        //s begins with starting vertex (1)
        //printf("Starting loop 8");
        s = malloc(vertices * sizeof(vertexid_t));
        s[0] = v1;
        s_count = 1;
        
        c_list[i] = find_weight(c, v1, v_list[i], weight_name->name);
        //choose min weight 
        min = inf;
        for(int j = 0; j < vertices; j++){
            //check if weight in v-s 
            //printf("starting loop 9");

            in_vertex_schema = 1;
            for(int k = 0; k < s_count; k++){
                if(v_list[j] == s[k])
                    in_vertex_schema = 0;
            }
             //calculate min in V-S 
            if(in_vertex_schema){
                if(c_list[j] < min){
                    min_index = j;
                }
            }
            else{
                continue;
            }
            
            //add vertex to s
            dij_w = j;
            s[s_count] = v_list[dij_w];
            s_count++;
            
            //now check if this edge is part of the shortest path 
            for(int l = 0; l < vertices; l++){
                //is v in v-s??
                //printf("starting loop 11");

                dij_v = l;
                in_vertex_schema = 1;
                for(int m = 0; m < s_count; m++){
                    if(v_list[dij_v] == s[m])
                        in_vertex_schema = 0;
                }
                //add to cost matrix, check if we found shortest path
                if(in_vertex_schema){
                    int another_weight = find_weight(c, v_list[dij_w], v_list[dij_v], weight_name->name);
                    if(c_list[dij_w] == inf || another_weight == inf){
                        //short circuiting, if D[dij_w], c[dij_w, dij_v] are inf, then dont need to do < 
                        c_list[dij_v] = min(c_list[dij_v], inf);
                        
                    }
                    else{
                        //if we hit this case, neither are inf, do addition and compare
                        if(c_list[dij_w] + another_weight < c_list[dij_v]){
                            p_list[dij_v] = v_list[dij_w];
                            c_list[dij_v] = c_list[dij_w] + another_weight;
                        }
                    }
                }
            }
        }
    }
    
    //make sssp_list to show vertices in shortest path
    sssp_list[0] = v2;
    sssp_list_count = 1;
    
    for(vertexid_t tmp = p_list[end]; tmp != v1; tmp = p_list[get_id_index(tmp, v_list, vertices)]){
        //printf("Starting loop 13");
        sssp_list[sssp_list_count] = tmp;
        sssp_list_count++;
        
    }
    
    sssp_list[sssp_list_count] = v1;
    sssp_list_count++;
    
    //final print statement 
    printf("Shortest path between %llu and %llu: [", v1, v2);
    //printf("Vertices included : [");
    for(int a = sssp_list_count-1; a >= 0; a--){
        if(a != 0)
            printf("%llu,", sssp_list[a]);
        else
            printf("%llu", sssp_list[a]);
    }
    printf("]\n");

    printf("Cost: %d\n",  c_list[end]);
    
    /* free vectors */
    free(p_list);
    free(v_list);
    free(c_list);
    free(sssp_list);
    free(s);

    return (1);
}