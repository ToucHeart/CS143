* 添加%option noyywrap来解决Undefined reference to yylex
* 添加%option stack来使用yy_push_stack
* 添加%option yylineno来使用行号
* 在正则表达式或者规则中引用其他正则表达式的名字要加{}
* grading下面是正确的输出，test-out/下面是自己的输出，unfilt是没有过滤信息的输出，out是过滤信息之后的输出

A non-escaped newline character may not appear in a string:

"This \

is OK" 		//这种是允许的，	

"This is not

OK"			//这种是不允许的

使用\可以将字符串分多行书写，\之后不能有空格，和c++中的用法一致，使用\\\\\n来匹配这种情况

* 把源代码中的非转义字符串\n转换成输出中的单个字符'\n'，使用\\\n来匹配，类似的还有\\\\\b
*
