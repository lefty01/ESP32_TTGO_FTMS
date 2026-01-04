#include <Arduino.h>   //Comment without space
#include <vector>

/*BlockComment*/
/**Doxygen*/

#define DEBUG_BEGIN(x)        Serial.begin(x)
#define DEBUG_PRINT(x)        Serial.print(x)
#define DEBUG_PRINTLN(x)      Serial.println(x)

int add(int a,int b){return a+b;}

void foo( int x,  int y )
{
    if(x>0){
      Serial.println("positive");
    } else{
        Serial.println("non-positive");
    }
}

class Test {
public:
  Test():value(0){}
  void set(int v){value=v;}
  int get( ) const { return value; }
private:
  int value;
};

void loop(){
  //AnotherComment
  int result=add( 1,2 );
  foo(3,4);
  if (result) delay(50);
  delay(1000);
}

void setup(){
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("ready");
}
