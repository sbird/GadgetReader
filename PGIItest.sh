#!/bin/bash
$1 $2 1 > PGIIhead_out.test
diff PGIIhead_out.test $3
