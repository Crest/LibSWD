/*
 * $Id$
 *
 * Serial Wire Debug Open Library.
 * Library Body File.
 *
 * Copyright (C) 2010-2011, Tomasz Boleslaw CEDRO (http://www.tomek.cedro.info)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Tomasz Boleslaw CEDRO nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.*
 *
 * Written by Tomasz Boleslaw CEDRO <cederom@tlen.pl>, 2010-2011;
 *
 */

/** \file libswd_dap.c DAP, DP, AP Operation Routines. */

#include <libswd.h>

/*******************************************************************************
 * \defgroup swd_dap High-level SWD Debug Access Port operations.
 * High level functions in general call lower level functions that append
 * queue with specific commands and payload, but also react on received data.
 * They operate on data pointers where target data is being stored.
 * Operation can be SWD_OPERATION_QUEUE_APPEND for queueing only the command
 * for later execution, or SWD_OPERATION_EXECUTE to queue command, flush it
 * into the interface driver (target read/write) and react on its result before
 * function returns.
 * Return values: negative number on error, data on success.
 ******************************************************************************/

/** Debug Access Port Reset sends 50 CLK with TMS high that brings both
 * SW-DP and JTAG-DP into reset state.
 * \param *swdctx swd context pointer.
 * \param operation type (SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE).
 * \return number of elements processed or SWD_ERROR_CODE code on failure.
 */
int swd_dap_reset(swd_ctx_t *swdctx, swd_operation_t operation){
 swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: Executing swd_dap_reset(swdctx=0x%p, operation=%s)\n", (void*)swdctx, swd_operation_string(operation));

 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT;
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;  

 int res, qcmdcnt=0, tcmdcnt=0;
 res=swd_bus_setdir_mosi(swdctx);
 if (res<0) return res;
 res=swd_cmd_enqueue_mosi_dap_reset(swdctx);
 if (res<1) return res;
 qcmdcnt+=res;

 if (operation==SWD_OPERATION_ENQUEUE){
  return qcmdcnt;
 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_cmdq_flush(swdctx, operation);
  if (res<0) return res;
  tcmdcnt+=res;
  return qcmdcnt+tcmdcnt;
 } else return SWD_ERROR_BADOPCODE;
}

/** Activate SW-DP by sending out RESET and JTAG-TO-SWD sequence on SWDIOTMS line.
 * \param *swdctx swd context.
 * \return number of control bytes executed, or error code on failre.
 */
int swd_dap_select(swd_ctx_t *swdctx, swd_operation_t operation){
 swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: Executing swd_dap_activate(swdctx=0x%p, operation=%s)\n", (void*)swdctx, swd_operation_string(operation));

 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT;
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;

 int res, qcmdcnt=0, tcmdcnt=0;
 res=swd_bus_setdir_mosi(swdctx);
 if (res<0) return res;
 res=swd_cmd_enqueue_mosi_jtag2swd(swdctx);
 if (res<0) return res;
 qcmdcnt=res;

 if (operation==SWD_OPERATION_ENQUEUE){
  return qcmdcnt;       
 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_cmdq_flush(swdctx, operation);
  if (res<0) return res;
  tcmdcnt=+res;
  return qcmdcnt+tcmdcnt;
 } else return SWD_ERROR_BADOPCODE;
}

/** Macro: Read out IDCODE register and return its value on function return.
 * \param *swdctx swd context pointer.
 * \param operation operation type.
 * \return Number of elements processed or SWD_ERROR code error on failure.
 */
int swd_dp_read_idcode(swd_ctx_t *swdctx, swd_operation_t operation, int **idcode){
 swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: Executing swd_dp_read_idcode(swdctx=0x%p, operation=%s)\n", (void*)swdctx, swd_operation_string(operation));

 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT; 
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
         return SWD_ERROR_BADOPCODE;

 int res, cmdcnt=0;
 char APnDP, RnW, addr, cparity, *ack, *parity;

 APnDP=0;
 RnW=1;
 addr=SWD_DP_IDCODE_ADDR;

 res=swd_bus_write_request(swdctx, SWD_OPERATION_ENQUEUE, &APnDP, &RnW, &addr);
 if (res<1) return res;
 cmdcnt=+res;

 if (operation==SWD_OPERATION_ENQUEUE){
  res=swd_bus_read_ack(swdctx, operation, (char**)&swdctx->qlog.read.ack);
  if (res<1) return res;
  cmdcnt=+res;
  res=swd_bus_read_data_p(swdctx, operation, idcode, (char**)&swdctx->qlog.read.parity);
  if (res<1) return res;
  cmdcnt=+res;
  return cmdcnt;

 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_bus_read_ack(swdctx, operation, &ack);
  if (res<1) return res;
  cmdcnt+=res;
  res=swd_bus_read_data_p(swdctx, operation, idcode, &parity);
  if (res<0) return res;
  cmdcnt+=res;
  res=swd_bin32_parity_even(*idcode, &cparity); 
  if (res<0) return res;
  swdctx->log.dp.idcode=**idcode;
  if (cparity!=*parity) return SWD_ERROR_PARITY;
 swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: swd_dp_read_idcode(swdctx=0x%p, operation=%s, *idcode=0x%X/%s).\n", (void*)swdctx, swd_operation_string(operation), **idcode, swd_bin32_string(*idcode));
  return cmdcnt;
 } else return SWD_ERROR_BADOPCODE;
}

/** Macro: Reset target DAP, select SW-DP, read out IDCODE.
 * This is the proper SW-DP initialization as stated by ARM Information Center.
 * \param *swdctx swd context pointer.
 * \param operation type (SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE).
 * \return Target's IDCODE, or SWD_ERROR_CODE on failure.
 */ 
int swd_dap_detect(swd_ctx_t *swdctx, swd_operation_t operation, int **idcode){
 int res;
 res=swd_dap_select(swdctx, operation);
 if (res<1) return res;
 res=swd_dap_reset(swdctx, operation);
 if (res<1) return res;
 res=swd_dp_read_idcode(swdctx, operation, idcode);
 if (res<0) return res;
 return SWD_OK;
}

/** Macro: Generic read of the DP register.
 * \param *swdctx swd context to work on.
 * \param operation can be SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE.
 * \param addr is the address of the DP register to read.
 * \param **data is the pointer to data where result will be stored.
 * \return number of elements processed or SWD_ERROR_CODE on failure.
 */
int swd_dp_read(swd_ctx_t *swdctx, swd_operation_t operation, char addr, int **data){
 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT; 
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;

 int res, cmdcnt=0;
 char APnDP, RnW, cparity, *ack, *parity;

 APnDP=0;
 RnW=1;

 res=swd_bus_write_request(swdctx, SWD_OPERATION_ENQUEUE, &APnDP, &RnW, &addr);
 if (res<1) return res;
 cmdcnt=+res;

 if (operation==SWD_OPERATION_ENQUEUE){
  res=swd_bus_read_ack(swdctx, operation, (char**)&swdctx->qlog.read.ack);
  if (res<1) return res;
  cmdcnt=+res;
  res=swd_bus_read_data_p(swdctx, operation, data, (char**)&swdctx->qlog.read.parity);
  if (res<1) return res;
  cmdcnt=+res;
  return cmdcnt;

 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_bus_read_ack(swdctx, operation, &ack);
  if (res<1) return res;
  cmdcnt+=res;
  res=swd_bus_read_data_p(swdctx, operation, data, &parity);
  if (res<0) return res;
  cmdcnt+=res;
  res=swd_bin32_parity_even(*data, &cparity); 
  if (res<0) return res;
  if (cparity!=*parity) return SWD_ERROR_PARITY;
  swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: swd_dp_read(swdctx=0x%p, operation=%s, addr=0x%X, *data=0x%X/%s).\n", (void*)swdctx, swd_operation_string(operation), addr, **data, swd_bin32_string(*data));
  return cmdcnt;
 } else return SWD_ERROR_BADOPCODE;
} 

/** Macro function: Generic write of the DP register.
 * \param *swdctx swd context to work on.
 * \param operation can be SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE.
 * \param addr is the address of the DP register to write.
 * \param *data is the pointer to data to be written.
 * \return number of elements processed or SWD_ERROR code on failure.
 */
int swd_dp_write(swd_ctx_t *swdctx, swd_operation_t operation, char addr, int *data){
 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT; 
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;

 int res, cmdcnt=0;
 char APnDP, RnW, cparity, *ack, *parity;

 APnDP=0;
 RnW=0;

 res=swd_bus_write_request(swdctx, SWD_OPERATION_ENQUEUE, &APnDP, &RnW, &addr);
 if (res<1) return res;
 cmdcnt=+res;

 swd_bin32_parity_even(data, (char *)&swdctx->qlog.write.parity);

 if (operation==SWD_OPERATION_ENQUEUE){
  res=swd_bus_read_ack(swdctx, operation, (char**)&swdctx->log.write.ack);
  if (res<1) return res;
  cmdcnt=+res;
  res=swd_bus_write_data_ap(swdctx, operation, data);
  if (res<1) return res;
  cmdcnt=+res;
  return cmdcnt;

 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_bus_read_ack(swdctx, operation, &ack);
  if (res<1) return res;
  cmdcnt+=res;
  res=swd_bus_write_data_ap(swdctx, operation, data);
  if (res<0) return res;
  cmdcnt+=res;
  swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: swd_dp_write(swdctx=0x%p, operation=%s, addr=0x%X, *data=0x%X/%s).\n", (void*)swdctx, swd_operation_string(operation), addr, *data, swd_bin32_string(data));
  return cmdcnt;
 } else return SWD_ERROR_BADOPCODE;
} 


/** Macro function: Generic read of the AP register.
 * \param *swdctx swd context to work on.
 * \param operation can be SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE.
 * \param addr is the address of the AP register to read.
 * \param **data is the pointer to data where result will be stored.
 * \return number of elements processed or SWD_ERROR code on failure.
 */
int swd_ap_read(swd_ctx_t *swdctx, swd_operation_t operation, char addr, int **data){
 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT; 
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;

 int res, cmdcnt=0;
 char APnDP, RnW, cparity, *ack, *parity;

 APnDP=1;
 RnW=1;

 res=swd_bus_write_request(swdctx, SWD_OPERATION_ENQUEUE, &APnDP, &RnW, &addr);
 if (res<1) return res;
 cmdcnt=+res;

 if (operation==SWD_OPERATION_ENQUEUE){
  res=swd_bus_read_ack(swdctx, operation, (char**)&swdctx->qlog.read.ack);
  if (res<1) return res;
  cmdcnt=+res;
  res=swd_bus_read_data_p(swdctx, operation, data, (char**)&swdctx->qlog.read.parity);
  if (res<1) return res;
  cmdcnt=+res;
  return cmdcnt;

 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_bus_read_ack(swdctx, operation, &ack);
  if (res<1) return res;
  cmdcnt+=res;
  res=swd_bus_read_data_p(swdctx, operation, data, &parity);
  if (res<0) return res;
  cmdcnt+=res;
  res=swd_bin32_parity_even(*data, &cparity); 
  if (res<0) return res;
  if (cparity!=*parity) return SWD_ERROR_PARITY;
  swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: swd_ap_read(swdctx=0x%p, command=%s, addr=0x%X, *data=0x%X/%s).\n", (void*)swdctx, swd_operation_string(operation), addr, **data, swd_bin32_string(*data));
  return cmdcnt;
 } else return SWD_ERROR_BADOPCODE;
} 

/** Macro function: Generic write of the AP register.
 * \param *swdctx swd context to work on.
 * \param operation can be SWD_OPERATION_ENQUEUE or SWD_OPERATION_EXECUTE.
 * \param addr is the address of the AP register to write.
 * \param *data is the pointer to data to be written.
 * \return number of elements processed or SWD_ERROR code on failure.
 */
int swd_ap_write(swd_ctx_t *swdctx, swd_operation_t operation, char addr, int *data){
 if (swdctx==NULL) return SWD_ERROR_NULLCONTEXT; 
 if (operation!=SWD_OPERATION_ENQUEUE && operation!=SWD_OPERATION_EXECUTE)
  return SWD_ERROR_BADOPCODE;

 int res, cmdcnt=0;
 char APnDP, RnW, cparity, *ack, *parity;

 APnDP=1;
 RnW=0;

 res=swd_bus_write_request(swdctx, SWD_OPERATION_ENQUEUE, &APnDP, &RnW, &addr);
 if (res<1) return res;
 cmdcnt=+res;

 swd_bin32_parity_even(data, (char *)&swdctx->qlog.write.parity);

 if (operation==SWD_OPERATION_ENQUEUE){
  res=swd_bus_read_ack(swdctx, operation, (char**)&swdctx->qlog.write.ack);
  if (res<1) return res;
  cmdcnt=+res;
  res=swd_bus_write_data_ap(swdctx, operation, data);
  if (res<1) return res;
  cmdcnt=+res;
  return cmdcnt;

 } else if (operation==SWD_OPERATION_EXECUTE){
  res=swd_bus_read_ack(swdctx, operation, &ack);
  if (res<1) return res;
  cmdcnt+=res;
  res=swd_bus_write_data_ap(swdctx, operation, data);
  if (res<0) return res;
  cmdcnt+=res;
  swd_log(swdctx, SWD_LOGLEVEL_INFO, "SWD_I: swd_ap_write(swdctx=0x%p, operation=%s, addr=0x%X, *data=0x%X/%s).\n", (void*)swdctx, swd_operation_string(operation), addr, *data, swd_bin32_string(data));
  return cmdcnt;
 } else return SWD_ERROR_BADOPCODE;
} 

/** @} */
