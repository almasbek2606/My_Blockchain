#include <sys/wait.h>
#include "header.h"

// Function to check if two head blocks are equal(e.g. synced)
bool equal(block* first, block* second)
{
    block* block1 = first;
    block* block2 = second;
    if ((!block1 || !block1->bid) && (!block2 || !block2->bid)) return true;
    if (((!block1 || !block1->bid) && (block2 || block2->bid)) || ((block1 || block1->bid) && (!block2 || !block2->bid))) return false;
    if ((block1->nid == NULL_HEAD && block2->nid == NULL_HEAD)) return true;
    while ((block1 && block1->bid) && (block2 && block2->bid))
    {
        if (strcmp(block1->bid, block2->bid) != 0) return false;
        block1 = block1->next;
        block2 = block2->next;
    }
    if ((!block1 && block2) || (block1 && !block2)) return false;
    return true;
}

// Function to check if all the nodes are synced among each other
bool is_synced(node* head)
{
    if (get_size(head) <= 1) return true;               // if size is 0 or 1, the blockchain is synced
    node* current = head->next;                         // check all blocks with blocks of head node
    block* b = head->block;
    while (current && current->nid != NULL_HEAD)
    {
        if (!equal(b, current->block)) return false;    // if blocks of a node are not the same as blocks of head, blockchain is not synced
        current = current->next;
    }
    return true;
}

// Function to return list of all blocks from the blockchain nodes without duplicates
string_list* get_set_of_blocks(node* head)
{
    if (!head || head->nid == NULL_HEAD) return NULL;
    node* current = head;
    string_list* set_list = (string_list*)malloc(sizeof(string_list));
    set_list->name = NULL;
    set_list->next = NULL;
    block* current_block;

    while (current && current->nid != NULL_HEAD)
    {
        current_block = current->block;
        while (current_block && current_block->bid)
        {
            add_to_string_list(&set_list, current_block->bid);
            current_block = current_block->next;
        }
        current = current->next;
    }
    return set_list;
}

// Function to sync all nodes
void sync_nodes(node** head)
{
    node* current = *head;
    if(!current || current->nid == NULL_HEAD)
    {
        write(1, NO_NODES, strlen(NO_NODES));
        return;
    }
    string_list* list = get_set_of_blocks(*head);
    string_list* list_node;
    while (current)
    {
        free_blocks(&current->block);
        current->block = init_blocks();
        list_node = list;
        while (list_node && list_node->name)
        {
            add_block(head, list_node->name, current->nid);
            list_node = list_node->next;
        }
        current = current->next;
    }
    free_string_list(list);
}
block* init_blocks()
{
    block* head = (block*)malloc(sizeof(block));                // init head block of a node
    if(!head)                                                   // check if memory is allocated
    {
        free(head);
        write(1, NO_RESOURSE, strlen(NO_RESOURSE));
        return NULL;
    }
    head->bid = NULL;
    head->nid = NULL_HEAD;
    head->next = NULL;
    return head;
}

// Function to check if a block with given bid and nid exists in the blockchain
bool exists_block(node* head, char* bid, int nid)
{
    if (!head || head->nid == NULL_HEAD) return false;
    node* current = head;
    block* current_block;
    while (current && current->nid != NULL_HEAD)
    {
        current_block = current->block;
        while (current_block && current_block->bid)
        {
            if (strcmp(current_block->bid, bid) == 0 && current_block->nid == nid) return true;
            current_block = current_block->next;
        }
        current = current->next;
    }
    return false;
}

// Function to add a block to the node with the given nid
void add_block(node** head, char* bid, int nid)
{
    node* current_node = *head;
    if(!current_node || current_node->nid == NULL_HEAD)          // if head is null
    {
        write(1, NO_NODES, strlen(NO_NODES));
        return;
    }

    if (exists_block(*head, bid, nid))                           // check if given block exists
    {
        write(1, BLOCK_EXISTS, strlen(BLOCK_EXISTS));
    }
    else
    {
        while (current_node && current_node->nid != nid)
        {
            current_node = current_node->next;
        }
        if (!current_node)
        {
            write(1, NODE_NOT_EXISTS, strlen(NODE_NOT_EXISTS));
            return;
        }
        if (current_node->block == NULL) current_node->block = init_blocks();
        block* current_block = current_node->block;
        block* tmp;
        if (current_block->nid == NULL_HEAD)                     // if the block is a head(null) block
        {
            current_block->bid = strdup(bid);
            current_block->nid = current_node->nid;
        }
        else
        {
            do{
                tmp = current_block;
                current_block = current_block->next;
            }while (current_block);                              // get to the last block

            block* newBlock = (block*)malloc(sizeof(block));     // create new block
            if (!newBlock)
            {
                free(newBlock);
                write(1, NO_RESOURSE, strlen(NO_RESOURSE));
                return;
            }
            newBlock->bid = strdup(bid);
            newBlock->nid = current_node->nid;
            newBlock->next = NULL;
            tmp->next = newBlock;
        }
    }
}

// Function to remove a block from the blockchain
void remove_block(node** head, char* bid)
{
    node* current = *head;
    if(!current || current->nid == NULL_HEAD)
    {
        write(1, NO_NODES, strlen(NO_NODES));
        return;
    }
    int counter = 0;
    while (current && current->nid != NULL_HEAD)
    {
        if (exists_block(*head, bid, current->nid)) counter++;
        if (counter > 0) break;
        current = current->next;
    }
    if (counter == 0)                                               // if there is no block with the given bid
    {
        write(1, BLOCK_NOT_EXISTS, strlen(BLOCK_NOT_EXISTS));
        return;
    }
    current = *head;
    block* current_block;
    block* prev_block;
    while (current && current->nid != NULL_HEAD)
    {
        current_block = current->block;
        if (current_block->nid != NULL_HEAD)                    // if node has no block
        {
            if (strcmp(current_block->bid, bid) == 0)           // if it is the head block
            {
                current->block = current_block->next;
                free(current_block->bid);
                free(current_block);
            }
            else
            {            
                while (current_block && current_block->bid)
                {
                    if (strcmp(current_block->bid, bid) == 0)
                    {
                        prev_block->next = current_block->next;
                        free(current_block->bid);
                        free(current_block);
                    }
                    prev_block = current_block;
                    current_block = current_block->next;
                }
            }
        }
        current = current->next;
    }
}

// Function to free all the blocks of a node
void free_blocks(block** head)
{
    block* current = *head;
    block* prev;
    while (current)
    {
        free(current->bid);
        prev = current;
        current = current->next;
        free(prev);
    }
}

// Function to print the bid of the blocks of a node for "ls -l" command
void print_block(block* current)
{
    if (!current || !current->bid)
    {
        printf("no blocks\n");
    }
    else 
    {
        while (current && current->bid)
        {
            printf("%s", current->bid);
            if(current->next) printf(", ");
            current = current->next;
        }
        printf("\n");
    }
}
void start(node** head)
{
    char* command;                                                      // container for a command line
    prompt_line(*head);                                                 // prompt to terminal
    while ((command = my_readline(0)) != NULL)                          // loop until "quit" command or NULL
    {
        if (!parse_command(head, command)) break;                       // parse command and break the loop if command is "quit"          
        prompt_line(*head);                                             // prompt to terminal
        free(command);
    }
    free(command);
}

// Function to handle "add" command
void command_add(node** head, string_array* words)
{
    int nid;
    if (words->size < 3)
    {
        write(1, NOT_ENOUGH_ARGS, strlen(NOT_ENOUGH_ARGS));
        return;
    }
    if (strcmp(words->array[1], "node") == 0)                           // if "add node ..."
    {
        if (words->size != 3)                                           // format is : "add node nid"
        {
            write(1, NOT_ENOUGH_ARGS, strlen(NOT_ENOUGH_ARGS));
        }
        else 
        {
            nid = my_atoi(words->array[2], strlen(words->array[2]));    // get node id by covnerting string to decimal
            add_node(head, nid);                                        // add node to the blockchain
        }
    }
    else if (strcmp(words->array[1], "block") == 0)                     // if "add block ..."
    {
        if (words->size != 4)                                           // format is : "add block bid nid"
        {
            write(1, NOT_ENOUGH_ARGS, strlen(NOT_ENOUGH_ARGS));
        }
        else 
        {
            if (strcmp(words->array[3], "*") == 0)                      // if "add block bid *"
            {
                node* current = *head;
                while (current && current->nid != NULL_HEAD)            // traverse all nodes
                {
                    nid = current->nid;                                 // get node id
                    add_block(head, words->array[2], nid);              // add block to all nodes one by one
                    current = current->next;
                }
            }
            else                                                        // if "add block bid nid"
            {
                nid = my_atoi(words->array[3], strlen(words->array[3])); // get node id by covnerting string to decimal
                add_block(head, words->array[2], nid);                  // add block to the blockchain
            }
        }
    }
    else                                                                // if "add [unknown word] ..."
    {
        write(1, UNKNOWN, strlen(UNKNOWN));
    }
}

// Function to handle "rm" command
void command_rm(node** head, string_array* words)
{
    if (words->size < 3)
    {
        write(1, NOT_ENOUGH_ARGS, strlen(NOT_ENOUGH_ARGS));
        return;
    }
    int nid;
    if (strcmp(words->array[1], "node") == 0)                           // if "rm node ..."
    {
        if (strcmp(words->array[2], "*") == 0)                          // if "rm node *"
        {
            node* current = *head;
            *head = NULL;                                               // set head to null
            free_node(&current);                                        // free the memory
        }
        else                                                            // if "rm node nid"
        {
            nid = my_atoi(words->array[2], strlen(words->array[2]));
            remove_node(head, nid);
        }
    }
    else if (strcmp(words->array[1], "block") == 0)                     // if "rm block ..."
    {
        if (strcmp(words->array[2], "*") == 0)                          // if "rm block *"
        {
            node* current = *head;                                      
            block* current_block;
            while (current && current->nid != NULL_HEAD)
            {
                current_block = current->block;
                current->block = NULL;
                free_blocks(&current_block);
                current = current->next;
            }
        }
        else                                                            // if "rm block bid"
        {
            remove_block(head, words->array[2]);
        }
    }
    else                                                                // if "rm [unknown word]"
    {
        write(1, UNKNOWN, strlen(UNKNOWN));
    }
}

// Function to handle add/rm commands
void add_rm(node** head, char* command)
{
    string_array* words = my_split(command, " ");
    if (strcmp(words->array[0], "add") == 0)                            // if "add ..."
    {
        command_add(head, words);
    }
    else if (strcmp(words->array[0], "rm") == 0)                        // if "rm ..."
    {
        command_rm(head, words);
    }
    else
    {
        write(1, UNKNOWN, strlen(UNKNOWN));
    }
    free_string_array(words);                                           // free the array of strings
}

// Function to parse the command and call corresponding functions
bool parse_command(node** head, char* command)
{
    if (strcmp(command, QUIT) == 0)                                     // "quit" command
    {
        save_data(*head);                                            // save all data to data.txt
        return false;
    }
    else if (strcmp(command, SYNC) == 0)                                // "sync" command
    {
        sync_nodes(head);                                               // synchronize all nodes
    }
    else if (strcmp(command, LIST) == 0)                                // "ls" command
    {
        print_nodes(*head, false);                                      // print all nodes
    }
    else if (strcmp(command, LIST_L) == 0)                              // "ls -l" command
    {
        print_nodes(*head, true);                                       // print all nodes with blocks
    }
    else 
    {
        add_rm(head, command);                                          // handle add/rm commands
    }
    return true;
}
node* load_data()
{
    int fd = open("./backup.txt", O_RDONLY);                          // open the backup file
    node* head = init_nodes();                                      // init head node
    if (!head) return NULL;
    string_array* words;                                            // array to store the words of a line
    char* line;
    int nid;
    while ((line = my_readline(fd)) != NULL)                        // use my_readline to get line by line
    {
        words = my_split(line, " ");                                // split the line into words
        nid = my_atoi(words->array[0], strlen(words->array[0]));    // convert first word to decimal
        add_node(&head, nid);                                       // add a node with the given nid
        if(words->size != 1)                                        // if there's more than one word in the line
        {
            for (int i = 1; i < words->size; i++)                   // get words starting from the second one
            {
                add_block(&head, words->array[i], nid);             // add the blocks one by one
            }
        }
        free(line);                                                 // free the line
        free_string_array(words);                                   // free the array
    }
    close(fd);                                                      // close the file
    return head;                                                    // return head node of the blockchain
}
// custom memset which fills the buffer with specific value
void my_memset(char* buff, char val, int size)
{
    int index = 0;
    while (index < size)
    {
        buff[index] = val;
        index += 1;
    }
}

// custom bzero implementation which uses custom memset
void my_bzero(void* buff, size_t size)
{
    my_memset(buff, 0, size);
}

// custom my_split made in BootCamp C Quest08
string_array* my_split(char* a, char* b) 
{
    int lenA = strlen(a);                                               //get length of ptr a
    int lenB = strlen(b);                                               //get length of ptr b

    if (lenA == 0 || lenB == 0) return NULL;                            //if either of a or b is empty            
    
    int size = 0;                                                       //size of the returning string_array
    int index = 0;                                                      //index for the string_array elements
    
    for (int i = 0; i < lenA; i++)                                      //count number of separators in the text
    {                                     
        if (a[i] == *b) size++;
    }
    size+=1;                                                            //+1 because one separators divides the text into 2, 
                                                                        //another +1 is for null terminator
    string_array* arr = (string_array*)malloc(sizeof(string_array*));   //initialize string_array
    arr->size = size;                                                   //size of the array
    arr->array = (char**)malloc(sizeof(char*) * size);                  //malloc the array
    
    char* piece = strtok(a, b);                                         //pointer for each piece of the divided text                                           
    char* word;
    while (piece != NULL)                                               //while there are pieces
    {   
        word = strdup(piece);
        arr->array[index] = (char*)malloc(sizeof(char) * strlen(word));                                                              
        strcpy(arr->array[index], word);                               //save the piece 
        piece = strtok(NULL, b);                                        //get the next piece
        free(word);
        index++;                                                        //increment the index
    }
    free(piece);
    return arr;                                                         //return array
}

// Custom string to integer/long
int my_atoi(char* number_string, int size)
{
    int index = 0;
    int res = 0;
    while (index < size && number_string[index])
    {
        if (number_string[index] >= '0' && number_string[index] <= '9')
        {
            res *= 10;
            res += number_string[index] - '0';
        }
        index++;
    }
    return res;
}

// Function to reverse a string
char* my_reverse(char* ptr){
    int len = strlen(ptr);
    if (len <= 1) return ptr;
    int i = len - 1;  
    char* s = (char*)malloc(sizeof(char) * (len + 1));  
    s[len]='\0';
    while (ptr && *ptr !='\0')
    {
        s[i] = *ptr;
        ptr++;
        i--;
    }
    free(ptr);
    return s;
}

// Custom integer to string with specific base number
char* my_itoa(int val, int base)
{
    char* ptr = (char*)malloc(sizeof(char) * 100);
    my_bzero(ptr, 100);
    if (base < 2 || base > 16) return ptr;
    int num, i = 0;
    if(val < 0) num = -val;
    else num = val;
    if (num == 0)
    {
        ptr[i++] = '0';
        ptr[i] = '\0';
        return ptr;
    }
    while (num != 0)
    {
        int r = num % base;
        if (r >= 10) ptr[i] = 65 + (r - 10);
        else ptr[i] = 48 + r;
        num /= base;
        i++;
    }
    if (val < 0 && base == 10) ptr[i++] = '-';
    char* res = (char*)malloc(strlen(ptr));
    strcpy(res, ptr);
    res = my_reverse(res);
    free(ptr);
    return res;
}

void free_string_array(string_array* words)
{
    for (int i = 0; i < words->size; i++)
    {
        free(words->array[i]);
    }
    free(words->array);
    free(words);
}
#define READLINE_READ_SIZE 512

// Function to get the index of the start of the next line
int get_next_index(char* leftover, int* offset)
{
    int index = 0;
    while (leftover[index] != '\0')
    {
        if (leftover[index] == '\n')
        {
            break;
        }
        else if (leftover[index] == '\\' && leftover[++index] == 'n')
        {
            *offset = 1;
            break;
        }
        index++;
    }
    return index;
}

// Function to read from the fd and return the content
char* get_next_content(int fd)
{
    char* content = (char*)malloc(sizeof(char) * (READLINE_READ_SIZE + 1));
    // my_bzero(content, strlen(content));
    read(fd, content, READLINE_READ_SIZE);
    content[READLINE_READ_SIZE] = '\0';
    return content;
}

// Function to move the leftover string after getting a line
// Example: before -> leftover = "First line\nSecond line"
//          after  -> leftover = "Second line"
void move_string(char* leftover, int index, int offset)
{
    char* buff = (char*)malloc(strlen(leftover) - index + offset);
    my_bzero(buff, strlen(buff));
    strcpy(buff, &leftover[index+offset]);
    my_bzero(leftover, strlen(leftover));
    if (buff[0] == '\n')
    {
        if (strlen(buff) == 1)
        {
            free(buff);
            my_bzero(leftover, strlen(leftover));
            return;
        }
        else
        {
            buff += 1;
        }
    }
    strcpy(leftover, buff);
    free(buff);
}

// my_readline function to read from fd and return the content line by line
char* my_readline(int fd)
{
    if (fd < 0) return NULL;
    static char leftover[READLINE_READ_SIZE + 1];               // static variable to save leftover content
    char* content = get_next_content(fd);                       // read READLINE_READ_SIZE bytes of content from the fd
    strcat(leftover, content);                                  // append the leftover with content
    free(content);
    if (leftover[0] == '\n')                                     // if the first line is empty, move to the second line
    {
        move_string(leftover, 1, 0);
    }
    else if (strlen(leftover) == 0)                              // if the content from fd is empty, return NULL
    {
        free(content);
        return NULL;
    }
    
    unsigned int index = 0;
    int offset = 0;                                             // offset is needed if there are symbols "\n" in the text
                                                                // because it is two spaces, rather than one space in case of '\n'
    index = get_next_index(leftover, &offset);                  // get end-line index and offset

    if (index >= strlen(leftover) && leftover[index] != EOF)     // if READLINE_READ_SIZE is less than the length of a line
    {
        while (index >= strlen(leftover))                        // read from the fd until getting the whole line
        {
            content = get_next_content(fd);
            if (strlen(content) == 0)                            // if content is empty it means EOF is reached
            {
                free(content);
                break;
            }
            strcat(leftover, content);                          // append content to leftover
            index = get_next_index(leftover, &offset);          // get enf-line index
            free(content);
        }
    }

    char* line = malloc(sizeof(char) * (index + 1 - offset));   // init a line string
    strncpy(line, leftover, index - offset);                    // copy first n(index-offset) characters from leftover to line

    move_string(leftover, index, offset);                       // move leftover
    free(content);
    if (strlen(line) == 0) free(line);                           // if line is empty, free the memory
    return line;
    // return strlen(line) > 0 ? line : NULL;                      // return line if it is not empty, else return NULL
}
node* init_nodes()
{
    node* head = (node*)malloc(sizeof(node));               // init the first node in blockchain
    if (!head)                                              // if failed to alloc memory, return NULL
    {
        free(head);
        write(1, NO_RESOURSE, strlen(NO_RESOURSE));
        return NULL;
    }
    head->block = init_blocks();                            // init blocks of the node
    head->next = NULL;
    head->nid = NULL_HEAD;                                  // assign NULL_HEAD to head node id
    return head;
}

// Function to check if a node with given nid exists in the blockchain
bool exists_node(node** head, int nid)
{
    node* current = *head;
    if (!current || current->nid == NULL_HEAD) return false;            // it means no nodes in the blockchain
    while (current && current->nid != NULL_HEAD)
    {
        if (current->nid == nid) return true;
        current = current->next;
    }
    return false;
}

// Function to add a node to the blockchain
void add_node(node** head, int nid)
{
    if (exists_node(head, nid))                             // check if given node already exists
    {
        write(1, NODE_EXISTS, strlen(NODE_EXISTS));
    }
    else 
    {
        node* tmp;
        node* current = *head;

        if (current == NULL)                           // if head node is not malloced with init_nodes() and is empty
        {
            current = (node*)malloc(sizeof(node));
            if(!current)
            {
                free(current);
                write(1, NO_RESOURSE, strlen(NO_RESOURSE));
                return;
            }
            current->block = init_blocks();
            current->nid = nid;
            current->next = NULL;
            *head = current;
        }
        else if (current->nid == NULL_HEAD)                      // if head node is malloced but empty
        {
            current->nid = nid;
        }
        else
        {
            do{                                             // get to the last node
                tmp = current;
                current = current->next;
            }while (current);

            node* newNode = (node*)malloc(sizeof(node));    // create a new node
            if (!newNode)                                   // check if memory is allocated correctly
            {
                free(newNode);
                write(1, NO_RESOURSE, strlen(NO_RESOURSE));
                return;
            }
            newNode->nid = nid;
            newNode->block = init_blocks();                     
            newNode->next = NULL;
            tmp->next = newNode;
        }
    }
}

// Function to remove a node with given nid from the blockchain
void remove_node(node** head, int nid)
{
    node* current = *head;
    if(!current || current->nid == NULL_HEAD)
    {
        write(1, NO_NODES, strlen(NO_NODES));
        return;
    }
    if (!exists_node(head, nid))                            // check if given node doesn't exist in the blockchain
    {
        write(1, NODE_NOT_EXISTS, strlen(NODE_NOT_EXISTS));
    }
    else
    {
        if (current->nid == nid)                            // if given node is the head node
        {
            *head = current->next;                          // move the head
            free_blocks(&current->block);
            free(current);
        }
        else 
        {
            node* prev = current;
            while (current && current->nid != NULL_HEAD)    // traverse all nodes
            {
                if (current->nid == nid)
                {
                    prev->next = current->next;             // jump over the node
                    free_blocks(&current->block);
                    free(current);
                }
                prev = current;
                current = current->next;
            }
        }
    }
}

// function to free all the nodes in the blockchain
void free_node(node** head)
{
    node* current = *head;
    node* prev = NULL;
    while (current)
    {
        free_blocks(&current->block);
        prev = current;
        current = current->next;
        free(prev);
    }
}

// Function to print node id and its blocks 
void print_node(node* n, bool with_blocks)
{
    printf("%d", n->nid);
    if (!with_blocks)
    {
        printf("\n");
    }
    else
    {
        printf(": ");
        print_block(n->block);
    }
}

// Function to get the number of nodes in the blockchain
int get_size(node* head)
{
    if(!head || head->nid == NULL_HEAD) return 0;
    node* current = head;
    int size = 0;
    while (current)
    {
        size++;
        current = current->next;
    }
    return size;
}

// Function to print all the nodes with or without blocks for "ls/ls -l" commands
void print_nodes(node* head, bool with_blocks)
{
    if (!head || head->nid == NULL_HEAD)
    {
        write(1, NO_NODES, strlen(NO_NODES));
    }
    else 
    {
        node* current = head;
        while (current)
        {
            print_node(current, with_blocks);
            current = current->next;
        }
    }
}
char* get_prompt(bool synced, int size)
{
    char* size_string = my_itoa(size, DEC_BASE);                    // get the string version of integer size
    char* prompt = malloc(strlen(size_string) + 6);                 // create prompt string
    strcpy(prompt, "[");                                            // prompt = "["
    strcat(prompt, synced ? "s" : "-");                             // prompt = "[s"
    strcat(prompt, size_string);                                    // prompt = "[s3"
    strcat(prompt, "]> ");                                          // prompt = "[s3]> "
    prompt[strlen(prompt)] = '\0';                                  // end the string
    free(size_string);
    return prompt;
}

// Function to prompt a line to the terminal
// with synced and size values
void prompt_line(node* head)
{
    bool synced = is_synced(head);                                  // check if nodes are synced
    int size = get_size(head);                                      // get new size of blockchain                                           
    char* prompt = get_prompt(synced, size);                        // get the prompt string
    write(1, prompt, strlen(prompt));                               // write the prompt string to stdout
    free(prompt);
}
int get_line_size(node* current)
{
    char* nid_string = my_itoa(current->nid, DEC_BASE);
    int size = 1 + strlen(nid_string);
    if (current->block && current->block->bid) size++;
    block* current_block = current->block;
    while(current_block && current_block->bid)
    {
        size += strlen(current_block->bid);
        if (current_block->next) size++;
        current_block = current_block->next;
    }
    free(nid_string);
    return size;
}

// Function to get one line: nid bid bid ...
char* get_line(node* current, size_t nbytes)
{
    char* line = malloc(nbytes);
    char* nid_string = my_itoa(current->nid, DEC_BASE);
    strcpy(line, nid_string);
    if (current->block && current->block->bid) strcat(line, " ");
    block* current_block = current->block;
    while (current_block && current_block->bid)
    {
        strcat(line, current_block->bid);
        if (current_block->next) strcat(line, " ");
        current_block = current_block->next;
    }
    line[nbytes-1] = '\n';
    free(nid_string);
    return line;
}

// Function to write the blockchain data to the given file
void write_blockchain(int fd, node* head)
{
    if (!head || head->nid == NULL_HEAD) return;            
    node* current = head;
    char* line;
    size_t size;
    while (current && current->nid != NULL_HEAD)
    {
        size = get_line_size(current);                      
        line = get_line(current, size);
        write(fd, line, size);
        free(line);
        current = current->next;
    }
    free(line);
} 

// Function to save the blockchain data to backup.txt
void save_data(node* head)
{
    int fd = open("./backup.txt", O_WRONLY | O_TRUNC);
    write_blockchain(fd, head);
    close(fd);
}
bool exists_in_list(string_list* list, char* word)
{
    string_list* current = list;
    while (current && current->name)
    {
        if(strcmp(current->name, word) == 0) return true;
        current = current->next;
    }
    return false;
}

// Function to add new node to linked list
void add_to_string_list(string_list** list, char* word)
{
    if(exists_in_list(*list, word)) return;
    string_list* current = *list;
    if (!current->name)
    {
        current->name = strdup(word);
        return;
    }
    string_list* tmp;
    do {
		tmp = current;
		current = current->next;
	} while (current);

    string_list* new_node = (string_list*)malloc(sizeof(string_list));
    new_node->name = strdup(word);
    new_node->next = NULL;
    tmp->next = new_node;
}

// Function to get size of the linked list
int get_list_size(string_list* list)
{
    string_list* current = list;
    int size = 0;
    while (current && current->name)
    {
        size++;
        current = current->next;
    }
    return size;
}

// Function to free the memory of linked list
void free_string_list(string_list* list)
{
    string_list* current = list;
    string_list* prev;
    while (current)
    {
        free(current->name);
        prev = current;
        current = current->next;
        free(prev);
    }
}

// Function to print linked list
void print_list(string_list* list)
{
    printf("List: ");
    string_list* current = list;
    while (current && current->name)
    {
        printf("%s", current->name);
        if(current->next) printf(", ");
        else printf("\n");
        current = current->next;
    }
}