/*

  Copyright (c) 2016 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include <assert.h>
#include <stdio.h>

#include "../libdill.h"

coroutine void sender1(int ch, int val) {
    int rc = chsend(ch, &val, sizeof(val), -1);
    assert(rc == 0);
}

coroutine void sender2(int ch, int val) {
    int rc = yield();
    assert(rc == 0);
    rc = chsend(ch, &val, sizeof(val), -1);
    assert(rc == 0);
}

coroutine void sender3(int ch, int val, int64_t deadline) {
    int rc = msleep(deadline);
    assert(rc == 0);
    rc = chsend(ch, &val, sizeof(val), -1);
    assert(rc == 0);
}

coroutine void receiver1(int ch, int expected) {
    int val;
    int rc = chrecv(ch, &val, sizeof(val), -1);
    assert(rc == 0);
    assert(val == expected);
}

coroutine void receiver2(int ch, int expected) {
    int rc = yield();
    assert(rc == 0);
    int val;
    rc = chrecv(ch, &val, sizeof(val), -1);
    assert(rc == 0);
    assert(val == expected);
}

coroutine void choosesender(int ch, int val) {
    struct chclause cl = {CHSEND, ch, &val, sizeof(val)};
    int rc = choose(&cl, 1, -1);
    assert(rc == 0);
}

coroutine void feeder(int ch, int val) {
    while(1) {
        int rc = chsend(ch, &val, sizeof(val), -1);
        if(rc == -1 && errno == ECANCELED) return;
        assert(rc == 0);
        rc = yield();
        if(rc == -1 && errno == ECANCELED) return;
        assert(rc == 0);
    }
}

struct large {
    char buf[1024];
};

int main() {
    int rc;
    int val;

    /* Trivial case. */
    int ch0 = channel(sizeof(int), 1);
    assert(ch0 >= 0);
    val = 555;
    struct chclause cls01[] = {{CHSEND, ch0, &val, sizeof(val)}};
    rc = choose(cls01, 1, -1);
    assert(rc == 0);
    val = 0;
    struct chclause cls02[] = {{CHRECV, ch0, &val, sizeof(val)}};
    rc = choose(cls02, 1, -1);
    assert(rc == 0);
    assert(val == 555);    

    /* Non-blocking receiver case. */
    int ch1 = channel(sizeof(int), 0);
    assert(ch1 >= 0);
    int hndl1 = go(sender1(ch1, 555));
    assert(hndl1 >= 0);
    struct chclause cls1[] = {{CHRECV, ch1, &val, sizeof(val)}};
    rc = choose(cls1, 1, -1);
    assert(rc == 0);
    assert(val == 555);
    hclose(ch1);
    rc = hclose(hndl1);
    assert(rc == 0);

    /* Blocking receiver case. */
    int ch2 = channel(sizeof(int), 0);
    assert(ch2 >= 0);
    int hndl2 = go(sender2(ch2, 666));
    assert(hndl2 >= 0);
    struct chclause cls2[] = {{CHRECV, ch2, &val, sizeof(val)}};
    rc = choose(cls2, 1, -1);
    assert(rc == 0);
    assert(val == 666);
    hclose(ch2);
    rc = hclose(hndl2);
    assert(rc == 0);

    /* Non-blocking sender case. */
    int ch3 = channel(sizeof(int), 0);
    assert(ch3 >= 0);
    int hndl3 = go(receiver1(ch3, 777));
    assert(hndl3 >= 0);
    val = 777;
    struct chclause cls3[] = {{CHSEND, ch3, &val, sizeof(val)}};
    rc = choose(cls3, 1, -1);
    assert(rc == 0);
    hclose(ch3);
    rc = hclose(hndl3);
    assert(rc == 0);

    /* Blocking sender case. */
    int ch4 = channel(sizeof(int), 0);
    assert(ch4 >= 0);
    int hndl4 = go(receiver2(ch4, 888));
    assert(hndl4 >= 0);
    val = 888;
    struct chclause cls4[] = {{CHSEND, ch4, &val, sizeof(val)}};
    rc = choose(cls4, 1, -1);
    assert(rc == 0);
    hclose(ch4);
    rc = hclose(hndl4);
    assert(rc == 0);

    /* Check with two channels. */
    int hndl5[2];
    int ch5 = channel(sizeof(int), 0);
    assert(ch5 >= 0);
    int ch6 = channel(sizeof(int), 0);
    assert(ch6 >= 0);
    hndl5[0] = go(sender1(ch6, 555));
    assert(hndl5 >= 0);
    struct chclause cls5[] = {
        {CHRECV, ch5, &val, sizeof(val)},
        {CHRECV, ch6, &val, sizeof(val)}
    };
    rc = choose(cls5, 2, -1);
    assert(rc == 1);
    assert(val == 555);
    hndl5[1] = go(sender2(ch5, 666));
    assert(hndl5 >= 0);
    rc = choose(cls5, 2, -1);
    assert(rc == 0);
    assert(val == 666);
    hclose(ch5);
    hclose(ch6);
    rc = hclose(hndl5[0]);
    assert(rc == 0);
    rc = hclose(hndl5[1]);
    assert(rc == 0);

    /* Test whether selection of in channels is random. */
    int ch7 = channel(sizeof(int), 0);
    assert(ch7 >= 0);
    int ch8 = channel(sizeof(int), 0);
    assert(ch8 >= 0);
    int hndl6[2];
    hndl6[0] = go(feeder(ch7, 111));
    assert(hndl6[0] >= 0);
    hndl6[1] = go(feeder(ch8, 222));
    assert(hndl6[1] >= 0);
    int i;
    int first = 0;
    int second = 0;
    int third = 0;
    for(i = 0; i != 100; ++i) {
        struct chclause cls6[] = {
            {CHRECV, ch7, &val, sizeof(val)},
            {CHRECV, ch8, &val, sizeof(val)}
        };
        rc = choose(cls6, 2, -1);
        assert(rc == 0 || rc == 1);
        if(rc == 0) {
            assert(val == 111);
            ++first;
        }
        if(rc == 1) {
            assert(val == 222);
            ++second;
        }
        int rc = yield();
        assert(rc == 0);
    }
    assert(first > 1 && second > 1);
    hclose(hndl6[0]);
    hclose(hndl6[1]);
    hclose(ch7);
    hclose(ch8);

    /* Test 'otherwise' clause. */
    int ch9 = channel(sizeof(int), 0);
    assert(ch9 >= 0);
    struct chclause cls7[] = {{CHRECV, ch9, &val, sizeof(val)}};
    rc = choose(cls7, 1, 0);
    assert(rc == -1 && errno == ETIMEDOUT);
    hclose(ch9);
    rc = choose(NULL, 0, 0);
    assert(rc == -1 && errno == ETIMEDOUT);

    /* Test two simultaneous senders vs. choose statement. */
    int ch10 = channel(sizeof(int), 0);
    assert(ch10 >= 0);
    int hndl7[2];
    hndl7[0] = go(sender1(ch10, 888));
    assert(hndl7[0] >= 0);
    hndl7[1] = go(sender1(ch10, 999));
    assert(hndl7[1] >= 0);
    val = 0;
    struct chclause cls8[] = {{CHRECV, ch10, &val, sizeof(val)}};
    rc = choose(cls8, 1, -1);
    assert(rc == 0);
    assert(val == 888);
    val = 0;
    rc = choose(cls8, 1, -1);
    assert(rc == 0);
    assert(val == 999);
    hclose(ch10);
    rc = hclose(hndl7[0]);
    assert(rc == 0);
    rc = hclose(hndl7[1]);
    assert(rc == 0);

    /* Test two simultaneous receivers vs. choose statement. */
    int ch11 = channel(sizeof(int), 0);
    assert(ch11 >= 0);
    int hndl8[2];
    hndl8[0] = go(receiver1(ch11, 333));
    assert(hndl8[0] >= 0);
    hndl8[1] = go(receiver1(ch11, 444));
    assert(hndl8[1] >= 0);
    val = 333;
    struct chclause cls9[] = {{CHSEND, ch11, &val, sizeof(val)}};
    rc = choose(cls9, 1, -1);
    assert(rc == 0);
    val = 444;
    rc = choose(cls9, 1, -1);
    assert(rc == 0);
    hclose(ch11);
    rc = hclose(hndl8[0]);
    assert(rc == 0);
    rc = hclose(hndl8[1]);
    assert(rc == 0);

    /* Choose vs. choose. */
    int ch12 = channel(sizeof(int), 0);
    assert(ch12 >= 0);
    int hndl9 = go(choosesender(ch12, 111));
    assert(hndl9 >= 0);
    struct chclause cls10[] = {{CHRECV, ch12, &val, sizeof(val)}};
    rc = choose(cls10, 1, -1);
    assert(rc == 0);
    assert(val == 111);
    hclose(ch12);
    rc = hclose(hndl9);
    assert(rc == 0);

    /* Choose vs. buffered channels. */
    int ch13 = channel(sizeof(int), 2);
    assert(ch13 >= 0);
    val = 999;
    struct chclause cls11[] = {{CHSEND, ch13, &val, sizeof(val)}};
    rc = choose(cls11, 1, -1);
    assert(rc == 0);
    struct chclause cls12[] = {{CHRECV, ch13, &val, sizeof(val)}};
    rc = choose(cls12, 1, -1);
    assert(rc == 0);
    assert(val == 999);
    hclose(ch13);

    /* Test whether allocating larger in buffer breaks previous in clause. */
    int ch15 = channel(sizeof(struct large), 1);
    assert(ch15 >= 0);
    int ch16 = channel(sizeof(int), 1);
    assert(ch16 >= 0);
    int hndl10 = go(sender2(ch16, 1111));
    assert(hndl10 >= 0);
    struct large lrg;
    struct chclause cls13[] = {
        {CHRECV, ch16, &val, sizeof(val)},
        {CHRECV, ch15, &lrg, sizeof(lrg)}
    };
    rc = choose(cls13, 2, -1);
    assert(rc == 0);
    assert(val == 1111);
    hclose(ch16);
    hclose(ch15);
    rc = hclose(hndl10);
    assert(rc == 0);

    /* Test transferring a large object. */
    int ch17 = channel(sizeof(struct large), 1);
    assert(ch17 >= 0);
    struct large large = {{0}};
    rc = chsend(ch17, &large, sizeof(large), -1);
    assert(rc == 0);
    struct chclause cls14[] = {{CHRECV, ch17, &lrg, sizeof(lrg)}};
    rc = choose(cls14, 1, -1);
    assert(rc == 0);
    hclose(ch17);

    /* Test that 'in' on done-with channel fires. */
    int ch18 = channel(sizeof(int), 0);
    assert(ch18 >= 0);
    rc = chdone(ch18);
    assert(rc == 0);
    struct chclause cls15[] = {{CHRECV, ch18, &val, sizeof(val)}};
    rc = choose(cls15, 1, -1);
    assert(rc == 0 && errno == EPIPE);
    hclose(ch18);

    /* Test expiration of 'deadline' clause. */
    int ch21 = channel(sizeof(int), 0);
    assert(ch21 >= 0);
    int64_t start = now();
    struct chclause cls17[] = {{CHRECV, ch21, &val, sizeof(val)}};
    rc = choose(cls17, 1, start + 50);
    assert(rc == -1 && errno == ETIMEDOUT);
    int64_t diff = now() - start;
    assert(diff > 30 && diff < 70);
    hclose(ch21);

    /* Test unexpired 'deadline' clause. */
    int ch22 = channel(sizeof(int), 0);
    assert(ch22 >= 0);
    start = now();
    int hndl11 = go(sender3(ch22, 4444, start + 50));
    assert(hndl11 >= 0);
    struct chclause cls18[] = {{CHRECV, ch22, &val, sizeof(val)}};
    rc = choose(cls17, 1, start + 1000);
    assert(rc == 0);
    assert(val == 4444);
    diff = now() - start;
    assert(diff > 30 && diff < 70);
    hclose(ch22);
    rc = hclose(hndl11);
    assert(rc == 0);

    return 0;
}

