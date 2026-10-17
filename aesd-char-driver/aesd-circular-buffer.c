/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"
#include "aesdchar.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    if ((buffer->entry[0].buffptr == NULL) && (buffer->out_offs == buffer->in_offs)) 
    {
        PDEBUG("Buffer empty");
        return NULL;
    }
    if (buffer->out_offs > buffer->in_offs)
    {
        PDEBUG("out idx is bigger than in idx");
        //return NULL;
    }

    PDEBUG("starting char offset is %ld", char_offset);
    PDEBUG("output buffer idx is %d", buffer->out_offs);
    size_t remaining_size = char_offset;
    uint8_t current_idx = buffer->out_offs; //sense this is searching and not poping data off leaving out idx alone and making copy
    struct aesd_buffer_entry *currentEntry;
    for(int idx = 0; idx < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; idx++)
    {
        currentEntry = &buffer->entry[current_idx]; //read next entry in the buffer
        if (remaining_size < currentEntry->size) //if smaller then the offset is correct entry
        {
            PDEBUG("Found offset");
            PDEBUG("char off set is %ld", remaining_size);
            *entry_offset_byte_rtn = remaining_size;
            return currentEntry;
        }
        remaining_size -= currentEntry->size; //subtract off this entry and moving to the next entry
        current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; //move to the next buffer idx
        //PDEBUG("current buff idx %d", buffer->out_offs);
        //PDEBUG("char off set is now %ld", remaining_size);
        if (current_idx == buffer->in_offs) //look for buffer wrap around
        {
            PDEBUG("Buffer index wrapped around");
            break;
        }
    } 
    
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    PDEBUG("top of function in_offs %d", buffer->in_offs);
    if (buffer->full == 1) //check if buffer full
    {
        buffer->entry[buffer->in_offs] = *add_entry;
        buffer->in_offs++; //move both buffer idx ahead one
        buffer->out_offs++;
        if (buffer->out_offs > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            buffer->out_offs = 0;
        }
        PDEBUG("Buffer full overwriting");
    }
    else
    {
        buffer->entry[buffer->in_offs] = *add_entry; //load in the entry
        PDEBUG("wrote IN buffer idx @ %d", buffer->in_offs);
        buffer->in_offs++; //move input idx
        if (buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) //check for full buffer
        {
            buffer->in_offs = 0; //move buffer in idx back to start
            buffer->full = 1; //mark full
            PDEBUG("Buffer now full");
        }
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
