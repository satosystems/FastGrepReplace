#! /bin/sh 
#
# check.sh - wkf self-check script
#    Sat, Nov 15 1997 by Kouichi ABE (WALL)
#
# $Id: check.sh,v 1.4 2005/08/31 10:10:10 kouichi Exp $
#

##############################################################################
#
# 各種コマンドの設定
#
##############################################################################
LD_LIBRARY_PATH="`dirname ${PWD}`:$LD_LIBRARY_PATH"
wkf=$1
diff=diff

##############################################################################
#
# 改行コードの扱いを調べる
#     (mnews の config.jsh の仕組みを利用)
#
##############################################################################
check_line_feed () {
  (echo "12345\c"; echo " ") > echotmp
  if grep c echotmp >/dev/null 2>&1; then
    n='-n'
    c=''
  else
    n=''
    c='\c'
  fi
  \rm -f echotmp
}

##############################################################################
#
# 漢字コード判定チェック関数
#
##############################################################################
guess_test ()
{
  if [ "\"`${wkf} -c ${file}`\"" = "\"${ans}\"" ]; then echo OK; else echo NG; fi
#  echo "\"`${wkf} -c ${file}`\""
#  echo "\"${ans}\"" 
}

##############################################################################
#
# 漢字コード変換チェック関数
#
##############################################################################
conv_test ()
{
  if ${cmd} $a | ${diff} - $b > /dev/null 2>&1; then echo OK; else echo NG; fi
}

##############################################################################
#
# 漢字コード判定チェック開始
#
##############################################################################
check_code ()
{
  echo "[ Code Check ]"

  echo $n "    ASCII Code Check ... $c"
  file=text.ascii
  ans=ascii
  guess_test

  echo $n "      JIS Code Check ... $c"
  file=text.jis
  ans=newjis
  guess_test

  echo $n "      EUC Code Check ... $c"
  file=text.euc
  ans=euc-jp
  guess_test

  echo $n "     SJIS Code Check ... $c"
  file=text.sjis
  ans=sjis
  guess_test

  echo $n "Ambiguous Code Check ... $c"
  file=coffee.txt
  ans='euc-jp/sjis'
  guess_test

  echo $n "    JIS X 0212 Check ... $c"
  file=x0212.jis
  ans='newjis+jisx0212'
  guess_test

  echo $n "  Illegal Code Check ... $c"
  file=.
  # エラーになるようなら ans=data でチェックすること
  ans=unknown
  guess_test

  echo
}

##############################################################################
#
# 漢字コード(JIS X 0208)変換チェック開始
#
##############################################################################
check_convert_x0208 ()
{
  echo "[ X0208: Conversion Check ]"

  #
  # JIS convert check
  #
  echo $n " JIS to  JIS ... $c"
  cmd="$wkf -j"
  a=text.jis
  b=text.jis
  conv_test

  echo $n " JIS to  EUC ... $c"
  cmd="$wkf -e"
  a=text.jis
  b=text.euc
  conv_test

  echo $n " JIS to SJIS ... $c"
  cmd="$wkf -s"
  a=text.jis
  b=text.sjis
  conv_test

  #
  # EUC convert check
  #
  echo $n " EUC to  JIS ... $c"
  cmd="$wkf -j"
  a=text.euc
  b=text.jis
  conv_test

  echo $n " EUC to  EUC ... $c"
  cmd="$wkf -e"
  a=text.euc
  b=text.euc
  conv_test

  echo $n " EUC to SJIS ... $c"
  cmd="$wkf -s"
  a=text.euc
  b=text.sjis
  conv_test

  #
  # SJIS convert check
  #
  echo $n "SJIS to  JIS ... $c"
  cmd="$wkf -j"
  a=text.sjis
  b=text.jis
  conv_test

  echo $n "SJIS to  EUC ... $c"
  cmd="$wkf -e"
  a=text.sjis
  b=text.euc
  conv_test

  echo $n "SJIS to SJIS ... $c"
  cmd="$wkf -s"
  a=text.sjis
  b=text.sjis
  conv_test

  echo
}

##############################################################################
#
# 漢字コード(JIS X 0201)変換チェック開始
#
##############################################################################
check_convert_x0201 ()
{
  echo "[ X0201: Conversion Check ]"

  #
  # JIS convert check
  #
  echo $n " JIS to  JIS ... $c"
  cmd="$wkf -j"
  a=x0201.jis
  b=x0201.x0208.jis
  conv_test

  echo $n " JIS to  EUC ... $c"
  cmd="$wkf -e"
  a=x0201.jis
  b=x0201.x0208.euc
  conv_test

  echo $n " JIS to SJIS ... $c"
  cmd="$wkf -s"
  a=x0201.jis
  b=x0201.x0208.sjis
  conv_test

  #
  # EUC convert check
  #
  echo $n " EUC to  JIS ... $c"
  cmd="$wkf -j"
  a=x0201.euc
  b=x0201.x0208.jis
  conv_test

  echo $n " EUC to  EUC ... $c"
  cmd="$wkf -e"
  a=x0201.euc
  b=x0201.x0208.euc
  conv_test

  echo $n " EUC to SJIS ... $c"
  cmd="$wkf -s"
  a=x0201.euc
  b=x0201.x0208.sjis
  conv_test

  #
  # SJIS convert check
  #
  echo $n "SJIS to  JIS ... $c"
  cmd="$wkf -j"
  a=x0201.sjis
  b=x0201.x0208.jis
  conv_test

  echo $n "SJIS to  EUC ... $c"
  cmd="$wkf -e"
  a=x0201.sjis
  b=x0201.x0208.euc
  conv_test

  echo $n "SJIS to SJIS ... $c"
  cmd="$wkf -s"
  a=x0201.sjis
  b=x0201.x0208.sjis
  conv_test

  echo
}

##############################################################################
#
# 漢字コード(JIS X 0212)変換チェック開始
#
##############################################################################
check_convert_x0212 ()
{
  echo "[ X0212: Conversion Check ]"

  #
  # JIS convert check
  #
  echo $n " JIS to  JIS ... $c"
  cmd="$wkf -j"
  a=x0212.jis
  b=x0212.jis
  conv_test

  echo $n " JIS to  EUC ... $c"
  cmd="$wkf -e"
  a=x0212.jis
  b=x0212.euc
  conv_test

  echo $n " JIS to SJIS ... $c"
  cmd="$wkf -s"
  a=x0212.jis
  b=x0212.sjis
  conv_test

  #
  # EUC convert check
  #
  echo $n " EUC to  JIS ... $c"
  cmd="$wkf -j"
  a=x0212.euc
  b=x0212.jis
  conv_test

  echo $n " EUC to  EUC ... $c"
  cmd="$wkf -e"
  a=x0212.euc
  b=x0212.euc
  conv_test

  echo $n " EUC to SJIS ... $c"
  cmd="$wkf -s"
  a=x0212.euc
  b=x0212.sjis
  conv_test

  echo
}

##############################################################################
#
# 漢字コード(JIS X 0208)変換チェック開始(Ａ→A 変換チェック)
#
##############################################################################
check_convert_x0208_ascii ()
{
  echo "[ X0208 -> ASCII: Conversion Check ]"
  echo
  echo " ==== Phase 1 ===="
  #
  # JIS convert check
  #
  echo $n " JIS to  JIS ... $c"
  cmd="$wkf -zj"
  a=alpha.jis
  b=alpha.ascii
  conv_test

  echo $n " JIS to  EUC ... $c"
  cmd="$wkf -ze"
  a=alpha.jis
  b=alpha.ascii
  conv_test

  echo $n " JIS to SJIS ... $c"
  cmd="$wkf -zs"
  a=alpha.jis
  b=alpha.ascii
  conv_test

  #
  # EUC convert check
  #
  echo $n " EUC to  JIS ... $c"
  cmd="$wkf -zj"
  a=alpha.euc
  b=alpha.ascii
  conv_test

  echo $n " EUC to  EUC ... $c"
  cmd="$wkf -ze"
  a=alpha.euc
  b=alpha.ascii
  conv_test

  echo $n " EUC to SJIS ... $c"
  cmd="$wkf -zs"
  a=alpha.euc
  b=alpha.ascii
  conv_test

  #
  # SJIS convert check
  #
  echo $n "SJIS to  JIS ... $c"
  cmd="$wkf -zj"
  a=alpha.sjis
  b=alpha.ascii
  conv_test

  echo $n "SJIS to  EUC ... $c"
  cmd="$wkf -ze"
  a=alpha.sjis
  b=alpha.ascii
  conv_test

  echo $n "SJIS to SJIS ... $c"
  cmd="$wkf -zs"
  a=alpha.sjis
  b=alpha.ascii
  conv_test

  echo

  echo " ==== Phase 2 ===="
  #
  # JIS convert check
  #
  echo $n " JIS to  JIS ... $c"
  cmd="$wkf -zj"
  a=dialy.jis
  b=dialy.jisz
  conv_test

  echo $n " JIS to  EUC ... $c"
  cmd="$wkf -ze"
  a=dialy.jis
  b=dialy.eucz
  conv_test

  echo $n " JIS to SJIS ... $c"
  cmd="$wkf -zs"
  a=dialy.jis
  b=dialy.sjisz
  conv_test

  #
  # EUC convert check
  #
  echo $n " EUC to  JIS ... $c"
  cmd="$wkf -zj"
  a=dialy.euc
  b=dialy.jisz
  conv_test

  echo $n " EUC to  EUC ... $c"
  cmd="$wkf -ze"
  a=dialy.euc
  b=dialy.eucz
  conv_test

  echo $n " EUC to SJIS ... $c"
  cmd="$wkf -zs"
  a=dialy.euc
  b=dialy.sjisz
  conv_test

  #
  # SJIS convert check
  #
  echo $n "SJIS to  JIS ... $c"
  cmd="$wkf -zj"
  a=dialy.sjis
  b=dialy.jisz
  conv_test

  echo $n "SJIS to  EUC ... $c"
  cmd="$wkf -ze"
  a=dialy.sjis
  b=dialy.eucz
  conv_test

  echo $n "SJIS to SJIS ... $c"
  cmd="$wkf -zs"
  a=dialy.sjis
  b=dialy.sjisz
  conv_test

  echo
}

##############################################################################
#
# はじまり
#
##############################################################################
check_line_feed;
check_code;
check_convert_x0208;
check_convert_x0201;
check_convert_x0212;
check_convert_x0208_ascii;

##############################################################################
#
# おしまい
#
##############################################################################
