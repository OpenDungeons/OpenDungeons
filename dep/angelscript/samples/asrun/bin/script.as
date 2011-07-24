/*

  Script must have 'int main()' or 'void main()' as the entry point.

  Some functions that are available:

   void           print(const string &in str);
   array<string> @getCommandLineArgs();

  Some objects that are available:

   string
   array<T>
   file

*/

string g_str = getDefaultString();

enum E
{
  VALUE1 = 20,
  VALUE2 = 30
}

class Test
{
  int a;
  int b;
  string c;
  void method()
  {
    print("In Test::method()\n");
  }
}

int main()
{
  E val = E(100);
  array<string> @args = getCommandLineArgs();

  print("Received the following args : " + join(args, "|") + "\n");

  Test t;
  Test @ht = t;
  t.method();
  
  array<int> a;
  array<int> @ha;
  
  function();

  // Garbage collection is automatic
  // Set up a circular reference to prove this
  {
    Link @link = Link();
    @link.next = link;
  }
  for( int n = 0; n < 1000; n++ )
  {
    Link @link = Link();
  }

  return 0;
}

void function()
{
  print("Currently in a different function\n");

  int n = 0;
  {
    int n = 1;
    string s = "hello";
    print(s + "\n");
  }
  {
    int n = 2;
  }
}

string getDefaultString()
{
  return "default";
}

class Link
{
  Link @next;
}
