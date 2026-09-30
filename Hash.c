#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Hash.h"


struct hash_table {
	int size;		//The variable size will be used to calculate the index of the array
    HashNode* items; // An array of hashnode items			
};

struct hashnode{
    int counter; // this variable is used to save how many times a word has been added to the hash table
	Pointer value; // we wiil save the word in the table
    HashNode next;  //Because we try to implement a hash table with seperate chaining this means that some elements may have the same index so we need the next node to save the elements
};


HashNode create_node(Pointer value){
    //Allocate memory for the hash node we create
    HashNode item = malloc(sizeof(*item));
    item->value = value;
    item->counter= 1 ; // initialize with one because when the node is created with a value
    item->next = NULL;

    return item;
}

HashTable create_hash_table(int countline){

    //Allocate memory for the hash table
    HashTable mytable= malloc(sizeof(*mytable));
    mytable->size=countline; 
    mytable->items = malloc(mytable->size * sizeof(HashNode));  

    //Itialize all the items with NULL 
    for (int i = 0; i < mytable->size; i++) {
        mytable->items[i] = NULL;  
    }

    return mytable;
}


int hash_string(HashTable table, Pointer value) { 
    long long p = 31; 
    long long m = table->size; 
    unsigned long long hash = 0;
    unsigned long long p_pow = 1;

    for (char* s = value; *s != '\0'; s++) {
        hash = (hash + ((unsigned char)*s) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }

    return (int) hash; // Return the final hash value cast to int
}

void hash_add(HashTable table, Pointer value) {

    if (value == NULL) {
        printf( "Null value \n");
        exit(1);
    }

    //calculating the index
    int index = hash_string(table, value);
    
    // Search for the value in the linked list at the given index
    //if the value already exists we have to increase the counter(frequency)
    HashNode current = table->items[index];
    while (current != NULL) {
        if (strcmp(current->value, value) == 0) { 
            current->counter++;
            return;
        }
        current = current->next;
    }

    //If the value doesnt already exists create the new hash node that we will add to the array
    HashNode new_node = create_node(value);

    // Insert the new node at the end of the linked list
    if (table->items[index] == NULL) {
        // If the list is empty add the new node 
        table->items[index] = new_node;
    } else {
        // If the list is not empty traverse to the end of the list and add it there
        HashNode last = table->items[index];
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_node;
    }


}

HashNode hash_find(HashTable table, Pointer value) {

    //Calculating the index to find the position of the array we will search
    int index = hash_string(table,value);
    
    // Traverse the list at the index to find the value
    HashNode temp = table->items[index];
    while (temp != NULL) {
        if (strcmp(temp->value,value)== 0) {
            return temp;  
        }
        temp = temp->next;
    }

    // Value not found
    return NULL;  
}

//returns the frequency of the word
int get_counter(HashTable table,Pointer value){
    HashNode node = hash_find(table, value);
    if (node != NULL) {
        return node->counter; // Return the counter if the node exists
    }
    return 0; // Return 0 if the value is not found

}


HashNode hash_first(HashTable table) {
	
    //the first node will be the first node in the fist not null list
	for (int i = 0; i < table->size; i++){
        if (table->items[i]!= NULL){
            return table->items[i];
        }
    }	

	return NULL;
}

HashNode hash_next(HashTable table, HashNode node) {
    if (node == NULL) {
        return NULL; 
    }

    // If there's a next node in the current chain return it
    if (node->next != NULL) {
        return node->next;
    }

    // Else move to the next node of the array
    int index = hash_string(table, node->value); // Find the index of the current node
    for (int i = index + 1; i < table->size; i++) {
        if (table->items[i] != NULL) {
            return table->items[i]; // Return the first non-NULL node of the array
        }
    }

    // If no further nodes are found return NULL
    return NULL;
}

Pointer hash_get_value(HashTable table, HashNode node){
    if (node->value !=NULL)
    {
        return node->value;
    }
    return NULL;
    
}

void delete_item(HashTable table, Pointer value ) {
    //Find the item we want to delete using the hash_string function
    int index = hash_string(table, value);
    HashNode current = table->items[index];
    HashNode prev = NULL; 
    while (current != NULL) {
        if (strcmp(current->value, value) == 0) {
            if (prev == NULL) {
                table->items[index] = current->next;  // Remove the first node
            } else {
                prev->next = current->next;  // Remove from middle or end
            }
            free(current);  // Free the node
            return;
        }
        prev = current;
        current = current->next;
    }
}


void delete_hash_table(HashTable table) {
    //Traverse every item in the table and delete
    for (int i = 0; i < table->size; i++) {
        if (table->items[i] != NULL) {
            while (table->items[i] != NULL) {
                HashNode next = table->items[i]->next;
                free(table->items[i]);        
                table->items[i] = next;  
            };  
        }
    }
    free(table->items);  
    free(table); 
}