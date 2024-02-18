#!/bin/bash
DATE=$(date -d "yesterday" '+%Y-%m-%d')
curl -L0 https://arquivos.b3.com.br/apinegocios/tickercsv/$DATE --output ./$DATE.zip
unzip *.zip
rm *.zip
mv *.txt $DATE.txt
./mktparser ./$DATE.txt ./$DATE.output.txt
