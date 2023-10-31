#include "sql_connection_pool11.h"
#include <unistd.h>
int main()
{
    connection_pool::GetInstance()->init("localhost","root","Mrchen613","webServer",3306,8,1);
    auto conn = connection_pool::GetInstance()->GetConnection();
    sleep(3);
    return 0;
}