#!/usr/bin/env bash

for no_records in 3000 6000; do
    echo "NUMBER OF RECORDS: $no_records"
    for record_length in 1 4 512 1024 4096 8196; do
        echo "GENERATING $no_records of LENGTH $record_length"
        ./program generate records "$no_records" "$record_length" 2>&1

        echo "COPYING $no_records RECORDS OF LENGTH $record_length USING SYSTEM FUNCTIONS"
        ./program copy records records_copy_sys "$no_records" "$record_length" sys

        echo "COPYING $no_records RECORDS OF LENGTH $record_length USING LIBRARY FUNCTIONS"
        ./program copy records records_copy_lib "$no_records" "$record_length" lib

        echo "SORTING $no_records RECORDS OF LENGTH $record_length USING SYSTEM FUNCTIONS"
        ./program sort records_copy_sys "$no_records" "$record_length" sys

        echo "SORTING $no_records RECORDS OF LENGTH $record_length USING LIBRARY FUNCTIONS"
        ./program sort records_copy_lib "$no_records" "$record_length" lib

    done
done