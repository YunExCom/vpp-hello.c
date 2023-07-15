// Copyright (c) 2018 Cisco and/or its affiliates.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hello.h"

DEFINE_VAPI_MSG_IDS_VPE_API_JSON;
DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON;
DEFINE_VAPI_MSG_IDS_L2_API_JSON;

vapi_error_e
show_version_cb (vapi_ctx_t ctx, void *caller_ctx,
			vapi_error_e rv, bool is_last,
			vapi_payload_show_version_reply * p) {
	int *c = caller_ctx;
	++*c;
	printf ("show_version: \n program\t: %s\n version\t: %s\n"
			" build in\t: %s\n build date\t: %s\n", 
			p->program, p->version, p->build_directory, p->build_date);
  	return VAPI_OK;
}

vapi_error_e
show_version (vapi_ctx_t ctx) {
	vapi_error_e rv = 0;
	int called = 0;
	vapi_msg_show_version *sv = vapi_alloc_show_version (ctx);

	GO(vapi_show_version (ctx, sv, show_version_cb, &called));

	return rv;
}

vapi_error_e
sw_interface_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
		      vapi_error_e rv, bool is_last,
		      vapi_payload_sw_interface_details * reply) {
	sw_interface_dump_ctx *dctx = callback_ctx;
	if (is_last) {
		dctx->last_called = true;
	}
	else {
		printf ("show_interface: [%u]: %s\n", reply->sw_if_index,
				reply->interface_name);
		size_t i = 0;
		for (i = 0; i < dctx->num_ifs; ++i) {
			if (dctx->sw_if_indexes[i] == reply->sw_if_index) {
				dctx->seen[i] = true;
			}
		}
	}
	++dctx->called;
	return VAPI_OK;
}

vapi_error_e
show_interfaces(vapi_ctx_t ctx) {
	vapi_error_e rv = 0;
	const size_t num_ifs = 5;
	u32 sw_if_indexes[num_ifs];
	clib_memset (&sw_if_indexes, 0xff, sizeof (sw_if_indexes));
	bool seen[num_ifs];
	sw_interface_dump_ctx dctx = { false, num_ifs, sw_if_indexes, seen, 0 };

	vapi_msg_sw_interface_dump *dump = vapi_alloc_sw_interface_dump (ctx);
	GO(vapi_sw_interface_dump (ctx, dump, sw_interface_dump_cb, &dctx));

	return rv;
}

const char * 
hello() {
	vapi_ctx_t ctx;
	static char *app_name = "vhello";
	static char *api_prefix = NULL;
	static const int max_outstanding_requests = 64;
	static const int response_queue_size = 32;

	vapi_error_e rv = 0;
	// Open a connect
	rv = vapi_ctx_alloc (&ctx);
   	if (rv != VAPI_OK) {
		return "alloc error";
	}
	rv = vapi_connect (ctx, app_name, api_prefix, 
				   	   max_outstanding_requests, response_queue_size, 
			   		   VAPI_MODE_BLOCKING, true);
	if (rv != VAPI_OK) {
		return "connect error";
	}

	// Show version
	show_version (ctx);
	printf ("\n");
	show_interfaces(ctx);

	// Free connection
 	rv = vapi_disconnect (ctx);
	if (rv != VAPI_OK) {
		return "disconnect error";
	}
	vapi_ctx_free (ctx);

	return "success\n";
}
