/*
Copyright (c) 2018, The Monero Project

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef POOL_H
#define POOL_H

void account_hr(double *avg, const char *address);
uint64_t account_balance(const char *address);

typedef struct payout_chart_t
{
    uint64_t payout_value;
    time_t payout_timestamp;
} payout_chart_t;

typedef struct total_payout_chart_t
{
    uint64_t total_payout_value;
    time_t total_payout_timestamp;
    uint64_t total_payout_count;
} total_payout_chart_t;

typedef struct hashrate_chart_t
{
    uint64_t hashrate_value;
    time_t hashrate_timestamp;
} hashrate_chart_t;

int get_24h_meanstddev_hr(const char *address, uint64_t *hrmean, uint64_t *hrstddev, uint64_t *chart_array_len_ptr, hashrate_chart_t **chart_array_ptr,
    uint64_t *hrmean2, uint64_t *hrstddev2, uint64_t *chart_array_len_ptr2, hashrate_chart_t **chart_array_ptr2,
    uint64_t *amnt, uint64_t *ts, uint64_t *chart_array_len_ptr3, payout_chart_t **chart_array_ptr3,
    uint64_t *tamnt, uint64_t *tts, uint64_t *tcnt, uint64_t *chart_array_len_ptr4, total_payout_chart_t **chart_array_ptr4);

uint64_t worker_count(const char *address);
void worker_list(char *list_start, char *list_end, const char *address);

#endif
