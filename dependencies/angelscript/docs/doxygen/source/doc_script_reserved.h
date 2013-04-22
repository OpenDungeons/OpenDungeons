/**

\page doc_reserved_keywords Reserved keywords and tokens

These are the keywords that are reserved by the language, i.e. they can't
be used by any script defined identifiers. Remember that the host application
may reserve additional keywords that are specific to that application.

<table cellspacing=0 cellpadding=0 border=0>
<tr>
<td width=100 valign=top><code>
and<br>
bool<br>
break<br>
case<br>
cast<br>
class<br>
const<br>
continue<br>
default<br>
do<br>
double<br>
</code></td>
<td width=100 valign=top><code>
else<br>
enum<br>
false<br>
final*<br>
float<br>
for<br>
from*<br>
funcdef<br>
get*<br>
if<br>
import<br>
</code></td>
<td width=100 valign=top><code>
in<br>
inout<br>
int<br>
interface<br>
int8<br>
int16<br>
int32<br>
int64<br>
is<br>
namespace<br>
not<br>
</code></td>
<td width=100 valign=top><code>
null<br>
or<br>
out<br>
override*<br>
private<br>
return<br>
set*<br>
shared*<br>
super*<br>
switch<br>
this*<br>
</code></td>
<td width=100 valign=top><code>
true<br>
typedef<br>
uint<br>
uint8<br>
uint16<br>
uint32<br>
uint64<br>
void<br>
while<br>
xor<br>
</code></td>
</tr>
</table>

<small>* Not really a reserved keyword, but is recognized by the compiler as a built-in keyword.</small>

These are the non-alphabetical tokens that are also used in the language syntax.

<table cellspacing=0 cellpadding=0 border=0>
<tr>
<td width=100 valign=top><code>
%*<br>
/<br>
%<br>
+<br>
-<br>
&lt;=<br>
&lt;<br>
&gt;=<br>
&gt;<br>
(<br>
</code></td><td width=100 valign=top><code>
)<br>
==<br>
!=<br>
?<br>
:<br>
=<br>
+=<br>
-=<br>
*=<br>
/=<br>
</code></td><td width=100 valign=top><code>
%=<br>
++<br>
--<br>
&<br>
,<br>
{<br>
}<br>
;<br>
|<br>
^<br>
</code></td><td width=100 valign=top><code>
~<br>
&lt;&lt;<br>
&gt;&gt;<br>
&gt;&gt;&gt;<br>
&=<br>
|=<br>
^=<br>
&lt;&lt;=<br>
&gt;&gt;=<br>
&gt;&gt;&gt;=<br>
</code></td><td width=100 valign=top><code>
.<br>
&amp;&amp;<br>
||<br>
!<br>
[<br>
]<br>
^^<br>
@ <br>
!is<br>
::<br>
</code></td>
</tr>
</table>

Other than the above tokens there are also numerical, string, identifier, and comment tokens.

<pre>
123456789
123.123e123
123.123e123f
0x1234FEDC
0d123987
0o1276
0b1010
'abc'
"abc"
"""heredoc"""
_Abc123
//
/*
*/
</pre>

The characters space (32), tab (9), carriage return (13), line feed (10), and the 
UTF8 byte-order-mark (U+FEFF) are all recognized as whitespace.

*/
