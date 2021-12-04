#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#define TOKSIZE 50

typedef struct {
  char report_date[TOKSIZE];
  char ticker_symbol[TOKSIZE];
  char update_action[TOKSIZE];
  char gross_trade_amount[TOKSIZE];
  char trade_quantity[TOKSIZE];
  char entry_time[TOKSIZE];
  char trade_id[TOKSIZE];
  char trade_session_id[TOKSIZE];
  char trade_date[TOKSIZE];
} TradeOrder;

typedef struct {
  char ticker_symbol[TOKSIZE];
  uint8_t hour;
  char date[12];
  uint16_t qty;
  uint16_t first_price;
  uint16_t last_price;
} TradeOrderBySymbol; // 74 bytes

typedef struct {
  size_t count;
  size_t size;
  TradeOrderBySymbol* arr;
} TradeOrderBySymbolArray;


TradeOrder line_to_trade_order(char* line) {
  char arr[9][TOKSIZE];
  TradeOrder trade_order_item;
  uint8_t i = 0;
  char *tok = strtok(line, ";");
  while(tok != NULL) {
    strcpy(arr[i], tok);
    arr[i][strcspn(arr[i], "\r\n")] = 0;
    tok = strtok(NULL, ";");
    i++;
  }
  strcpy(trade_order_item.report_date, arr[0]);
  strcpy(trade_order_item.ticker_symbol, arr[1]);
  strcpy(trade_order_item.update_action, arr[2]);
  strcpy(trade_order_item.gross_trade_amount, arr[3]);
  strcpy(trade_order_item.trade_quantity, arr[4]);
  strcpy(trade_order_item.entry_time, arr[5]);
  strcpy(trade_order_item.trade_id, arr[6]);
  strcpy(trade_order_item.trade_session_id, arr[7]);
  strcpy(trade_order_item.trade_date, arr[8]);
  // printf("ticker: %s\n", trade_order_item.ticker_symbol);
  return trade_order_item;
}

void remove_char(char* str, char chr) {
  uint8_t i, j, len;
  len = strlen(str);
  for (i = 0; i < len; i++) {
    if (str[i] == chr) {
      for (j = i; j < len; j++) {
        str[j] = str[j + 1];
      }
      len--;
      i--;
    }
  }
}

uint16_t parse_trade_amount(char* str) {
  char trade_amount_cents_str[50];
  strcpy(trade_amount_cents_str, str);
  remove_char(trade_amount_cents_str, ',');
  trade_amount_cents_str[strlen(trade_amount_cents_str) - 1] = '\0';
  uint16_t trade_amount_cents = atoi(trade_amount_cents_str);
  return trade_amount_cents;
}

uint8_t parse_entry_time(char* str) {
  char hour_str[3];
  memcpy(hour_str, str, 2);
  uint8_t hour_num = atoi(hour_str);
  return hour_num;
}

int write_to_result(FILE* result_file, char* ticker, uint8_t hour, uint16_t qty, uint16_t first_price, uint16_t last_price) {
  char result[2000];
  char hour_str[3];
  char first_price_str[9];
  char last_price_str[9];
  char qty_str[9];
  sprintf(hour_str, "%u", hour);
  sprintf(first_price_str, "%u", first_price);
  sprintf(last_price_str, "%u", last_price);
  sprintf(qty_str, "%u", qty);
  strcpy(result, ticker);
  strcat(result, ";");
  strcat(result, hour_str);
  strcat(result, ";");
  strcat(result, qty_str);
  strcat(result, ";");
  strcat(result, first_price_str);
  strcat(result, ";");
  strcat(result, last_price_str);
  strcat(result, "\r\n");
  return fputs(result, result_file);
}

int parse_trade_order(TradeOrderBySymbolArray* arr_struct, TradeOrder* trade_order, FILE* result_file) {
  uint8_t hour = parse_entry_time(trade_order->entry_time);
  uint16_t amount = parse_trade_amount(trade_order->gross_trade_amount);
  uint16_t qty = atoi(trade_order->trade_quantity);
  // printf("hour: %d, amount: %d\n", hour, amount);

  for (size_t i = 0; i < arr_struct->count; i++) { // this could be bad if too many different tickers, could use a binary search
    if (strcmp(arr_struct->arr[i].ticker_symbol, trade_order->ticker_symbol) == 0) {
      // printf("recalculating!!! %d %d\n", hour, arr_struct->arr[i].hour);
      uint16_t new_qty;
      uint8_t current_hour = arr_struct->arr[i].hour;
      arr_struct->arr[i].qty += qty;
      strcpy(arr_struct->arr[i].date, trade_order->trade_date);
      arr_struct->arr[i].last_price = amount;
      arr_struct->arr[i].hour = hour;
      if (hour > current_hour) {
        // write to file
        // TICKER;HOUR;QTY;FIRST;LAST
        int res = write_to_result(
            result_file,
            arr_struct->arr[i].ticker_symbol,
            arr_struct->arr[i].hour,
            arr_struct->arr[i].qty,
            arr_struct->arr[i].first_price,
            arr_struct->arr[i].last_price
        );
        // printf("%d result", res);
        // reset this line
        return res;
      }
      return 0;
    }
  }
  // insert new
  if (arr_struct->size <= arr_struct->count) { // should resize if array at limit
    // printf("realloc!!!%lu %lu\n", arr_struct->count, arr_struct->size);
    arr_struct->size += 1;
    arr_struct->arr = realloc(arr_struct->arr, arr_struct->size * sizeof(TradeOrder));
  }
  TradeOrderBySymbol new_trade;
  strcpy(new_trade.ticker_symbol, trade_order->ticker_symbol);
  strcpy(new_trade.date, trade_order->trade_date);
  new_trade.hour = hour;
  new_trade.qty = qty;
  new_trade.first_price = amount;
  new_trade.last_price = amount;
  arr_struct->arr[arr_struct->count] = new_trade;
  arr_struct->count += 1;
  int res = write_to_result(
    result_file,
    new_trade.ticker_symbol,
    new_trade.hour,
    new_trade.qty,
    new_trade.first_price,
    new_trade.last_price
  );
  // printf("inserted new trade...\n");
  return res;
}

void initialize_array(TradeOrderBySymbolArray* arr_struct) {
  arr_struct->count = 0;
  arr_struct->size = 10;
  arr_struct->arr = malloc(arr_struct->size * sizeof(TradeOrderBySymbol));
};
void free_array(TradeOrderBySymbolArray* arr_struct) {
  free(arr_struct->arr);
  arr_struct->arr = NULL;
  arr_struct->size = 0;
  arr_struct->count = 0;
}

int read_file(char* file_path, char* result_path) {
  TradeOrderBySymbolArray arr_struct;
  FILE *original_report_ptr;
  FILE *result_report_ptr;
  char *original_report_line = NULL;
  size_t len = 0;
  size_t nread;

  initialize_array(&arr_struct);
  original_report_ptr = fopen(file_path, "r");
  result_report_ptr = fopen(result_path, "w");
  if (!original_report_ptr) return 1;
  while((nread = getline(&original_report_line, &len, original_report_ptr)) != -1) {
    // printf("\nbuf %s\n", original_report_line);
    TradeOrder current = line_to_trade_order(original_report_line);
    // printf("current report date: %s\n", current.report_date);
    // printf("current ticker symbol: %s\n", current.ticker_symbol);
    // printf("current update action: %s\n", current.update_action);
    // printf("current gross trade amount: %s\n", current.gross_trade_amount);
    // printf("current trade quantity: %s\n", current.trade_quantity);
    // printf("current entry time: %s\n", current.entry_time);
    // printf("current trade id: %s\n", current.trade_id);
    // printf("current trade session id: %s\n", current.trade_session_id);
    // printf("current trade date: %s\n", current.trade_date);
    parse_trade_order(&arr_struct, &current, result_report_ptr);
    printf("count: %lu\n", arr_struct.count);
  }
  printf("\n");
  fclose(original_report_ptr);
  fclose(result_report_ptr);
  // for (int i = 0; i < arr_struct.count; i++) {
  //   printf("ticker: %s\n", arr_struct.arr[i].ticker_symbol);
  //   printf("qty: %d\n", arr_struct.arr[i].qty);
  //   printf("first_price: %d\n", arr_struct.arr[i].first_price);
  //   printf("last_price: %d\n", arr_struct.arr[i].last_price);
  //   printf("ticker_symbol: %s\n", arr_struct.arr[i].ticker_symbol);
  //   printf("date: %s\n", arr_struct.arr[i].date);
  //   printf("hour: %d\n", arr_struct.arr[i].hour);
  //   printf("-------------------\n");
  // }
  free_array(&arr_struct);
  return 0;
}

int main() {
  // int test = read_file("example-files/test.txt", "result.txt");
  int test = read_file("example-files/TradeIntraday_20211126_1.txt", "result.txt");
  printf("result: %d\n", test);
  return 0;
}

