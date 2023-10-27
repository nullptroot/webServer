#include "sql_connection_pool.h"
#include "../log/log.h"

int main()
{
    Log::get_instance()->init("connpoolTest",0);
    connection_pool::GetInstance()->init("127.0.0.1","root","Mrchen613",
                                            "webServer",3306,10,0);
    MYSQL *con = connection_pool::GetInstance()->GetConnection();
    const char *select = "select * from user";
    mysql_real_query(con,select,strlen(select));
    MYSQL_RES *result = mysql_store_result(con);

    while(MYSQL_ROW row = mysql_fetch_row(result))
    {
        std::cout<<row[0]<<" "<<row[1]<<std::endl;
    }
    return 0;
}