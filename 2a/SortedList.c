//Implementation of Linked List Functions

#include "SortedList.h"
#include <stdlib.h>
#include <string.h>

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    //If list is currently empty - simpy insert at the top
    if (list == NULL)
    {
        list = element;
        return;
    }

    //Else if the the head->key is larger than that of element
    else if (list->key != NULL && strcmp(list->key, element->key) >= 0)
    {
        element->next = list;
        list->prev = element;
        list = element;
        return;
    }

    SortedListElement_t *currentElement = list;
    while (currentElement->next != NULL && currentElement->next != list)
    {
        //If element's key is smaller than / equal tothe next element's key, then
        //insert it into the next position
        if (strcmp(currentElement->next->key, element->key) >= 0)
        {
            element->next = currentElement->next;
            element->prev = currentElement;
            currentElement->next->prev = element;
            currentElement->next = element;
            return;
        }
	else
	  currentElement = currentElement->next;
    }

    //Element-> key is the largest value in the list, add it to the end
    currentElement->next = element;
    element->prev = currentElement;
    element->next = list;
    list->prev = element;
    return;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t *element)
{
    //Check for corruption
    if (element == NULL)
        return 1;
    if (element->next->prev != element || element->prev->next != element)
        return 1;

    //Remove element from list - should I free memory?
    element->prev->next = element->next;
    element->next->prev = element->prev;

    return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    //Checking for valid inputs to the function
    if (key == NULL)
        return NULL;

    SortedListElement_t* currentElement = list->next;

    while (currentElement != NULL && currentElement != list)
    {
        if (currentElement->key != NULL && strcmp(currentElement->key, key) == 0)
            return currentElement;
        else
            currentElement = currentElement->next;
    }

    //If no element matches, return NULL
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
    int size;
    SortedListElement_t* currentElement = list;

    //Check if list is empty
    if (currentElement == NULL)
        return -1;
    else
        size = 0;

    //Loop through counting elements, as well as checking for corruption
    while (currentElement->next != NULL && currentElement->next != list)
    {
        //Checking for corruption
        if (currentElement->next->prev != currentElement || currentElement->prev->next != currentElement)
            return -1;
        
        //Increment size as currentElement->next is not NULL so 1 more element
        ++size;

        //Move ahead in the list
        currentElement = currentElement->next;
    }

    return size;
}
