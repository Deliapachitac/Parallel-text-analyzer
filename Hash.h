#include <stdbool.h>
#include <assert.h>
#include <stdbool.h> 

// Pointer to an object of any type.
typedef void* Pointer;

//Pointers to the structs
typedef struct hashnode* HashNode;
typedef struct hash_table* HashTable;


// Creating functions
HashNode create_node(Pointer value);
HashTable create_hash_table(int countline);

//Function to find the index of the array 
int hash_string(HashTable table,Pointer value);

//Function that inserts the value into the array
void hash_add(HashTable table, Pointer value);

//Finds the value in O(1) complexity 
HashNode hash_find(HashTable table, Pointer value);

//finds the first node of the has table 
HashNode hash_first(HashTable table);

//finds the next node of the hash
//because the hash table is implemented with separate chaining the next node will be the next from the list 
//if the list has ended then the node will be from the table in the struct
HashNode hash_next(HashTable table, HashNode node) ;

//Function that returns the counter(frequency)
int get_counter(HashTable table,Pointer value);

//functions for returning the value
Pointer hash_get_value(HashTable table, HashNode node);

// Deleting functions
void delete_item(HashTable table, Pointer value);
void delete_hash_table(HashTable table);