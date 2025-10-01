set(CONVERSION_PROCS_SRCS
  cyrillic_and_mic
  euc_cn_and_mic
  euc_jp_and_sjis
  euc_kr_and_mic
  euc_tw_and_big5
  euc2004_sjis2004
  latin_and_mic
  latin2_and_win1250
  utf8_and_big5
  utf8_and_cyrillic
  utf8_and_euc_cn
  utf8_and_euc_jp
  utf8_and_euc_kr
  utf8_and_euc_tw
  utf8_and_euc2004
  utf8_and_gb18030
  utf8_and_gbk
  utf8_and_iso8859
  utf8_and_iso8859_1
  utf8_and_johab
  utf8_and_sjis
   utf8_and_sjis2004
  utf8_and_uhc
  utf8_and_win
)

add_library(utils OBJECT ${CONVERSION_PROCS_SRCS})
