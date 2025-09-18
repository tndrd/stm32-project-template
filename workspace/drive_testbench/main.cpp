#include "app.h"

void boardShowException(Exception::Reason e=Exception::Unknown)
{
    GPIOC->BSRR = (1<<6) << 16;
    GPIOD->BSRR = (1<<12);
    while (1)
    {
//        GPIOD->ODR ^= (1 << 12);
        for (int w=1000000*((int)e+1); --w;);
        
    }
}

App *g_app = nullptr;

int main(void)
{
    rcc().configPll(168000000);
    
    App *app = 0L;
//    try
//    {
        app = new App;
        g_app = app;
        app->exec();   
//    }
//    catch (Exception::Reason e)
//    {
//        boardShowException(e);
//        while (1); // integer exception trap
//    }
//    catch (...)
//    {
//        boardShowException();
//        while (1); // unknown exception trap
//    }
    
    while (1); // fail exit trap
}

extern "C" void HardFault_Handler()
{
    boardShowException(Exception::BadSoBad);
}
