#!/bin/bash
DATE=$(date -d "Dec 10" '+%Y-%m-%d')
#DATE=$(date -d "yesterday" '+%Y-%m-%d')
curl -L0 https://arquivos.b3.com.br/apinegocios/tickercsv/$DATE --output ./$DATE.zip
unzzip *.zip
rm *.zip
mv *.txt $DATE.txt

