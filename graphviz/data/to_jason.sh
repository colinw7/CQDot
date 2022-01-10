#!/bin/csh -f

set file = $1

dot -Tjson $file > $file:r.json
