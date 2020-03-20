//
//    http://www.gnu.org/licenses/lgpl-2.1.html
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
//    USA

/*
 *---------------------------------------------------------------------------
 *| VERSION | AUTHOR            | DATE       | NOTE                         |
 *---------------------------------------------------------------------------
 *| 01      | Fan Chunquan      | 2017-05-30 |                              |
 *---------------------------------------------------------------------------
 */

#ifndef FWK_TASK_QUEUE_H_
#define FWK_TASK_QUEUE_H_

#include <fwk/task/mutex.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define QUEUE_MOD_NAME            TRACE_M_OS
#define QUEUE_ERR(s...)       base_trace_print(QUEUE_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define QUEUE_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define QUEUE_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)

#define FWK_QUEUE_RESERVED_NODES 1
#define FWK_QUEUE_MSG_DEF_NUM 8
#define FWK_QUEUE_MSG_DEF_LEN 128

/* queue name maximum size */
#define FWK_QUEUE_NAME_MAX_LEN 32

typedef void* fwk_queueID_t;

typedef struct {
	char name[FWK_QUEUE_NAME_MAX_LEN];
	uint16_t depth; /* max message number */
	uint16_t size; /* each message length */
	uint32_t maxBufSize; /* total messasge buffer size */
}fwk_queueAttr_t;
int fwk_createQueue(fwk_queueAttr_t* pQAttr, void* mid, fwk_queueID_t* qid);
int fwk_msgQSend(fwk_queueID_t qID, const void * data, uint16_t size, void* head, uint16_t headLen, int priority);
int fwk_msgQRecv(fwk_queueID_t qID, void * buffer, uint16_t bufsize, int timeout, void* head, uint16_t headLen, void* cid, void* mid);

/*
 * Create queue with fixed size.
 * Input:
 * 	name: a unique friendly name for the queue
 * 	qID: designated queue ID (0 if leave it for system allocation)
 * 	depth: how many items the queue can hold
 * 	size: size of each item
 * Output:
 * 	qID: allocated queue ID if not specified
 * Return:
 *  queue ID if created successfully, the queue ID shall be a positive integer.
 * 	or error code defined with fwk_taskQueueErrorCode_t
 * 		FWK_TASK_QUEUE_E_PARAM
 * 		FWK_TASK_QUEUE_E_INTERNAL
 */
extern int fwk_createFixsizeQueue(fwk_queueID_t * qID, char * name,
		uint16_t depth, uint16_t size);

/*
 * Create queue for variable sized data.
 * Input:
 * 	name: a unique friendly name for the queue
 * 	qID: designated queue ID (0 if leave it for system allocation)
 * 	maxBufSize: total size of memory to buffer data
 * Output:
 * 	qID: allocated queue ID if not specified
 * Return:
 *  queue ID if created successfully, the queue ID shall be a positive integer.
 * 	or error code defined with fwk_taskQueueErrorCode_t
 * 		FWK_TASK_QUEUE_E_PARAM
 * 		FWK_TASK_QUEUE_E_INTERNAL
 */
extern int fwk_createVarSizeQueue(fwk_queueID_t * qID, char * name,
		uint16_t maxBufSize);

/*
 * Send a data item to queue.
 * Parameters:
 * 	qID: identify the queue
 * 	data: data to send
 * 	size: size of the data to send
 * 	timeout: ticks timeout
 * Return:
 * 	0 if succeed
 * 	or error code defined with fwk_taskQueueErrorCode_t
 * 		FWK_TASK_QUEUE_E_PARAM
 * 		FWK_TASK_QUEUE_E_INTERNAL
 * 		FWK_TASK_QUEUE_E_FULL
 */
extern int fwk_sendToQueue(fwk_queueID_t qID, const void * data, uint16_t size,
		int timeout);

/*
 * Receive data from a queue.
 * Return:
 * 	valid data size if succeed
 * 	or error code defined with fwk_taskQueueErrorCode_t
 * 		FWK_TASK_QUEUE_E_PARAM
 * 		FWK_TASK_QUEUE_E_INTERNAL
 * 		FWK_TASK_QUEUE_E_NODATA
 * 		FWK_TASK_QUEUE_E_BUFOVERFLOW
 */
extern int fwk_receiveFromQueue(fwk_queueID_t qID, void * buffer,
		uint16_t bufsize, int timeout);

/*
 * Clear data in the queue
 */
extern int fwk_clearQueue(fwk_queueID_t qID);
extern void fwk_showQueue(fwk_queueID_t qID);
extern uint16_t fwk_getMsgSize(fwk_queueID_t qID);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* FWK_TASK_QUEUE_H_ */
