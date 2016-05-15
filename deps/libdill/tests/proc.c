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

#include "../libdill.h"

coroutine void foo(int a, int b) {
    assert(a == 3);
    assert(b == 4);
}

coroutine void bar(void) {
    int rc = msleep(now() + 100000);
    assert(rc == 0);
}

int main(void) {
    int h, rc, result;

    /* Launch the coroutine locally. */
    h = go(foo(3, 4));
    rc = msleep(now() + 50);
    assert(rc == 0);
    rc = hclose(h);
    assert(rc == 0);

    /* Launch the coroutine in a separate process. */
    h = proc(foo(3, 4));
    rc = msleep(now() + 100);
    assert(rc == 0);
    rc = hclose(h);
    assert(rc == 0);

    return 0;
}

