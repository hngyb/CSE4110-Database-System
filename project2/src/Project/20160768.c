#include <stdio.h>
#include <string.h>
#include <mysql.h>

#pragma comment(lib, "libmysql.lib")
#pragma warning(disable : 4996)

#define MAX 1024
const char *host = "localhost";
const char *user = "root";
const char *pw = "1234";
const char *db = "project";

char line[MAX];
FILE *fp;
int table_num;
int tuple_num;

MYSQL *connection = NULL;

int initialize_DB()
{
    /*
    txt파일로부터 DB를 초기화하는 함수이다.
    */
    char *query;
    int state = 0;
    int i;
    table_num = atoi(fgets(line, MAX, fp)); // table 개수
    tuple_num = atoi(fgets(line, MAX, fp)); // tuple 개수

    // create needed tables
    for (i = 0; i < table_num; i++)
    {
        query = fgets(line, MAX, fp);
        state = mysql_query(connection, query); // return 0 if the statement was successful
        puts(query);
        if (state != 0)
        {
            printf("CREATE TABLE ERROR: %s\n", query);
            return 1;
        }
    }

    // insert tuples
    for (i = 0; i < tuple_num; i++)
    {
        query = fgets(line, MAX, fp);
        state = mysql_query(connection, query);
        puts(query);
        if (state != 0)
        {
            printf("INSERT ERROR: %s\n", query);
            return 1;
        }
    }

    return 0;
}

char *replaceAll(char *s, const char *olds, const char *news)
{
    /*
    쿼리문에 사용자의 입력 변수를 적용하기 위한 함수이다.
    */
    char *result, *sr;
    size_t i, count = 0;
    size_t oldlen = strlen(olds);
    if (oldlen < 1)
        return s;
    size_t newlen = strlen(news);

    if (newlen != oldlen)
    {
        for (i = 0; s[i] != '\0';)
        {
            if (memcmp(&s[i], olds, oldlen) == 0)
                count++, i += oldlen;
            else
                i++;
        }
    }
    else
        i = strlen(s);

    result = (char *)malloc(i + 1 + count * (newlen - oldlen));
    if (result == NULL)
        return NULL;

    sr = result;
    while (*s)
    {
        if (memcmp(s, olds, oldlen) == 0)
        {
            memcpy(sr, news, newlen);
            sr += newlen;
            s += oldlen;
        }
        else
            *sr++ = *s++;
    }
    *sr = '\0';

    return result;
}

void type1(int k, char *brand_name, int type)
{
    char *query;
    char char_k[10];
    sprintf(char_k, "%d", k);

    int state = 0;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    switch (type)
    {
    case 1:
        /* 
        Show the sales trends for a particular brand over the past k years.

        query:
        select brand_name, sale_year, sum(price)
        from (vehicle join model_option using (option_id)) join sales using (VIN)
        where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now())
        group by brand_name, sale_year
        order by sale_year
        */
        query = "select brand_name, sale_year, sum(price) from (vehicle join model_option using (option_id)) join sales using (VIN) where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now()) group by brand_name, sale_year order by sale_year";
        query = replaceAll(query, "BRAND", brand_name);
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("  brand_name    year       amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s      %10s\n", sql_row[0], sql_row[1], sql_row[2]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 2:
        /*
        Then break these data out by gender of the buyer.
        
        query:
        select brand_name, sale_year, gender, sum(price)
        from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id)
        where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now())
        group by brand_name, sale_year, gender
        order by sale_year, gender
        */
        query = "select brand_name, sale_year, gender, sum(price) from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id) where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now()) group by brand_name, sale_year, gender order by sale_year, gender";
        query = replaceAll(query, "BRAND", brand_name);
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("  brand_name    year      gender     amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s      %6s    %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 3:
        /*Then by income range.

        query:
        select brand_name, sale_year, gender, 
            case 
                when annual_income between 0 and 19999 then 1 
                when annual_income between 20000 and 49999 then 2 
                when annual_income between 50000 and 79999 then 3 
                else 4 
                end income_range, 
        sum(price)
        from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id)
        where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now())
        group by brand_name, sale_year, gender, income_range
        order by sale_year, gender, income_range

        income_range 1: 0 <= income < 20000($)
        income_range 2: 20000($) <= income < 50000($)
        income_range 3: 50000($) <= income < 80000($)
        income_range 4: 80000($) <= income
        */
        query = "select brand_name, sale_year, gender, case when annual_income between 0 and 19999 then 1 when annual_income between 20000 and 49999 then 2 when annual_income between 50000 and 79999 then 3 else 4 end income_range, sum(price) from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id) where brand_name = 'BRAND' and year(sale_date) >= year(date_sub(now(), interval K year)) and year(sale_date) < year(now()) group by brand_name, sale_year, gender, income_range order by sale_year, gender, income_range";
        query = replaceAll(query, "BRAND", brand_name);
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("\nincome_range 1: 0 <= income < 20000($)\nincome_range 2: 20000($) <= income < 50000($)\nincome_range 3: 50000($) <= income < 80000($)\nincome_range 4: 80000($) <= income\n\n");
            printf("  brand_name    year      gender    income_range     amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s      %6s              %2s    %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3], sql_row[4]);
            }
            mysql_free_result(sql_result);
        }
        break;

    default:
        break;
    }
}

void type2(int k, int type)
{
    char *query;
    char char_k[10];
    sprintf(char_k, "%d", k);

    int state = 0;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    switch (type)
    {
    case 1:
        /* 
        Show sales trends for various brands over the past k months.

        query:
        select brand_name, sale_year, sale_month, sum(price)
        from (vehicle join model_option using (option_id)) join sales using (VIN)
        where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month))
        group by brand_name, sale_year, sale_month
        order by brand_name, sale_year, sale_month
        */
        query = "select brand_name, sale_year, sale_month, sum(price) from (vehicle join model_option using (option_id)) join sales using (VIN) where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month)) group by brand_name, sale_year, sale_month order by brand_name, sale_year, sale_month";
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("  brand_name    year    month     amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s       %2s    %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 2:
        /*
        Then break these data out by gender of the buyer.

        query:
        select brand_name, sale_year, sale_month, gender, sum(price)
        from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id)
        where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month))
        group by brand_name, sale_year, sale_month, gender
        order by brand_name, sale_year, sale_month, gender
        */
        query = "select brand_name, sale_year, sale_month, gender, sum(price) from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id) where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month)) group by brand_name, sale_year, sale_month, gender order by brand_name, sale_year, sale_month, gender";
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("  brand_name    year    month     gender     amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s       %2s     %6s    %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3], sql_row[4]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 3:
        /* 
        Then by income range.

        query:
        select brand_name, sale_year, sale_month, gender, 
            case 
                when annual_income between 0 and 19999 then 1 
                when annual_income between 20000 and 49999 then 2 
                when annual_income between 50000 and 79999 then 3 
                else 4 
                end income_range, 
        sum(price)
        from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id)
        where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month))
        group by brand_name, sale_year, sale_month, gender, income_range
        order by brand_name, sale_year, sale_month, gender, income_range

        income_range 1: 0 <= income < 20000($)
        income_range 2: 20000($) <= income < 50000($)
        income_range 3: 50000($) <= income < 80000($)
        income_range 4: 80000($) <= income
        */
        query = "select brand_name, sale_year, sale_month, gender, case when annual_income between 0 and 19999 then 1 when annual_income between 20000 and 49999 then 2 when annual_income between 50000 and 79999 then 3 else 4 end income_range, sum(price) from ((vehicle join model_option using (option_id)) join sales using (VIN)) join customer using (customer_id) where sale_date > last_day(date_sub(date_sub(now(), interval K month), interval 1 month)) and sale_date <= last_day(date_sub(now(), interval 1 month)) group by brand_name, sale_year, sale_month, gender, income_range order by brand_name, sale_year, sale_month, gender, income_range";
        query = replaceAll(query, "K", char_k);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("\nincome_range 1: 0 <= income < 20000($)\nincome_range 2: 20000($) <= income < 50000($)\nincome_range 3: 50000($) <= income < 80000($)\nincome_range 4: 80000($) <= income\n\n");
            printf("  brand_name    year    month     gender    income_range     amount($)\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%12s    %4s       %2s     %6s              %2s    %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3], sql_row[4], sql_row[5]);
            }
            mysql_free_result(sql_result);
        }
        break;

    default:
        break;
    }
}

void type3(char *supplier_name, char *date1, char *date2, int type)
{
    char *query;

    int state = 0;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    switch (type)
    {
    case 1:
        /*
        Find that transmissions made by supplier (company name) between two 
        given dates are defective.

        query:
        select supplier_name, part_id, part_type, made_date
        from supplies join supplier using (supplier_id)
        where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2'
        order by made_date
        */
        query = "select supplier_name, part_id, part_type, made_date from supplies join supplier using (supplier_id) where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2' order by made_date";
        query = replaceAll(query, "SUPPLIER", supplier_name);
        query = replaceAll(query, "DATE1", date1);
        query = replaceAll(query, "DATE2", date2);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("\nSupplier (company name): %s\nTransmissions made between %s and %s\n\n", supplier_name, date1, date2);
            printf("       supplier_name      part_id          part_type      made_date\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%20s     %8s    %15s     %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 2:
        /*
        Find the VIN of each car containing such a transmission and the
        customer to which it was sold.

        query:
        select supplier_name, part_id, part_type, made_date, VIN, customer_id, name as customer_name
        from ((produce join supplier using (supplier_id)) join vehicle using (VIN)) join customer using (customer_id)
        where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2'
        order by made_date, VIN
        */
        query = "select supplier_name, part_id, part_type, made_date, VIN, customer_id, name as customer_name from ((produce join supplier using (supplier_id)) join vehicle using (VIN)) join customer using (customer_id) where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2' order by made_date, VIN";
        query = replaceAll(query, "SUPPLIER", supplier_name);
        query = replaceAll(query, "DATE1", date1);
        query = replaceAll(query, "DATE2", date2);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("\nSupplier (company name): %s\nTransmissions made between %s and %s\n\n", supplier_name, date1, date2);
            printf("       supplier_name      part_id          part_type      made_date           VIN    customer_id    customer_name\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%20s     %8s    %15s     %10s    %10s        %7s       %10s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3], sql_row[4], sql_row[5], sql_row[6]);
            }
            mysql_free_result(sql_result);
        }
        break;

    case 3:
        /*
        Find the dealer who sold the VIN and transmission for each vehicle
        containing these transmissions

        query:
        select supplier_name, part_id, part_type, made_date, VIN, dealer_id, dealer_name
        from (((produce join supplier using (supplier_id)) join vehicle using (VIN)) join sales using (VIN)) join dealer using (dealer_id)
        where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2'
        order by made_date, VIN, dealer_id
        */
        query = "select supplier_name, part_id, part_type, made_date, VIN, dealer_id, dealer_name from (((produce join supplier using (supplier_id)) join vehicle using (VIN)) join sales using (VIN)) join dealer using (dealer_id) where supplier_name = 'SUPPLIER' and part_type = 'Transmission' and made_date between 'DATE1' and 'DATE2' order by made_date, VIN, dealer_id";
        query = replaceAll(query, "SUPPLIER", supplier_name);
        query = replaceAll(query, "DATE1", date1);
        query = replaceAll(query, "DATE2", date2);
        state = mysql_query(connection, query);

        if (state == 0)
        {
            sql_result = mysql_store_result(connection);
            printf("\nSupplier (company name): %s\nTransmissions made between %s and %s\n\n", supplier_name, date1, date2);
            printf("       supplier_name      part_id          part_type      made_date           VIN      dealer_id           dealer_name\n");
            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                printf("%20s     %8s    %15s     %10s    %10s       %8s       %15s\n", sql_row[0], sql_row[1], sql_row[2], sql_row[3], sql_row[4], sql_row[5], sql_row[6]);
            }
            mysql_free_result(sql_result);
        }
        break;

    default:
        break;
    }
}

void type4(int k, int year)
{
    /*
    Find the top k brands by dollar-amount sold by the year.

    query:
    select sale_year, brand_name, sum(price) as dollar_amount
    from (vehicle join sales using (VIN)) join model_option using (option_id)
    where sale_year = 'YEAR'
    group by brand_name
    order by dollar_amount desc limit K
    */
    char *query;
    int state = 0;
    char char_k[10];
    char char_year[5];
    sprintf(char_k, "%d", k);
    sprintf(char_year, "%d", year);
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    query = "select sale_year, brand_name, sum(price) as dollar_amount from (vehicle join sales using (VIN)) join model_option using (option_id) where sale_year = 'YEAR' group by brand_name order by dollar_amount desc limit K";
    query = replaceAll(query, "K", char_k);
    query = replaceAll(query, "YEAR", char_year);

    state = mysql_query(connection, query);

    if (state == 0)
    {
        sql_result = mysql_store_result(connection);
        printf("\nTOP %d brands (%d)\n\n", k, year);
        printf("sale_year     brand_name       dollar-amount\n");
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            printf("     %4s     %10s       %13s\n", sql_row[0], sql_row[1], sql_row[2]);
        }
        mysql_free_result(sql_result);
    }
}

void type5(int k, int year)
{
    /*
    Find the top k brands by unit sales by the year.

    query:
    select sale_year, brand_name, count(distinct VIN) as unit_sales
    from (sales join vehicle using (VIN)) join model_option using (option_id)
    where sale_year = 'YEAR'
    group by brand_name
    order by unit_sales desc limit K
    */
    char *query;
    int state = 0;
    char char_k[10];
    char char_year[5];
    sprintf(char_k, "%d", k);
    sprintf(char_year, "%d", year);
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    query = "select sale_year, brand_name, count(distinct VIN) as unit_sales from (sales join vehicle using (VIN)) join model_option using (option_id) where sale_year = 'YEAR' group by brand_name order by unit_sales desc limit K";
    query = replaceAll(query, "K", char_k);
    query = replaceAll(query, "YEAR", char_year);

    state = mysql_query(connection, query);

    if (state == 0)
    {
        sql_result = mysql_store_result(connection);
        printf("\nTOP %d brands (%d)\n\n", k, year);
        printf("sale_year     brand_name       unit_sales\n");
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            printf("     %4s     %10s       %10s\n", sql_row[0], sql_row[1], sql_row[2]);
        }
        mysql_free_result(sql_result);
    }
}

void type6()
{
    /*
    In what month(s) do convertibles sell best?

    query:
    with t as (select sale_month, count(distinct VIN) as c_sale
    from (sales join vehicle using (VIN)) join model_option using (option_id)
    where body_style = 'Convertible' group by sale_month order by count(distinct VIN) desc limit 1)
    select sale_month, body_style, count(distinct VIN)
    from (sales join vehicle using (VIN)) join model_option using (option_id)
    where body_style = 'Convertible'
    group by sale_month, body_style
    having count(distinct VIN) >= (select c_sale from t)
    order by sale_month
    */
    char *query;
    int state = 0;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    query = "with t as (select sale_month, count(distinct VIN) as c_sale from (sales join vehicle using (VIN)) join model_option using (option_id) where body_style = 'Convertible' group by sale_month order by count(distinct VIN) desc limit 1) select sale_month, body_style, count(distinct VIN) from (sales join vehicle using (VIN)) join model_option using (option_id) where body_style = 'Convertible' group by sale_month, body_style having count(distinct VIN) >= (select c_sale from t) order by sale_month";
    state = mysql_query(connection, query);

    if (state == 0)
    {
        sql_result = mysql_store_result(connection);
        printf("\nTOP month(s) convertibles sell best\n\n");
        printf("month      body_style       unit_sales\n");
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            printf(" %4s     %10s       %10s\n", sql_row[0], sql_row[1], sql_row[2]);
        }
        mysql_free_result(sql_result);
    }
}

void type7()
{
    /*
    Find those dealers who keep a vehicle in inventory for the longest average time.

    query:
    with t as (select dealer_id, avg(datediff(out_date, in_date)) as avg_time 
    from inventory
    group by dealer_id
    order by avg_time desc limit 1)
    select dealer_id, dealer_name, avg(datediff(out_date, in_date))
    from inventory join dealer using (dealer_id)
    group by dealer_id
    having avg(datediff(out_date, in_date)) >= (select avg_time from t);
    */

    char *query;
    int state = 0;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    query = "with t as (select dealer_id, avg(datediff(out_date, in_date)) as avg_time from inventory group by dealer_id order by avg_time desc limit 1) select dealer_id, dealer_name, avg(datediff(out_date, in_date)) from inventory join dealer using (dealer_id) group by dealer_id having avg(datediff(out_date, in_date)) >= (select avg_time from t)";
    state = mysql_query(connection, query);

    if (state == 0)
    {
        sql_result = mysql_store_result(connection);
        printf("dealer_id        dealer_name      avg_time(day)\n");
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            printf(" %8s    %15s       %10s\n", sql_row[0], sql_row[1], sql_row[2]);
        }
        mysql_free_result(sql_result);
    }
}

void clrscr()
{
    system("cls||clear");
}

void show_interface()
{
    /*
    user interface to handle each TYPE
    */
    clrscr();
    while (1)
    {
        int choice = -1;
        int subChoice = -1;
        char brand_name[20];
        char supplier_name[20];
        int k = -1;
        int quit = 1;
        char date1[11], date2[11];
        int year = -1;
        int end = 0;

        printf("\n---------- SELECT QUERY TYPES ----------\n\n");
        printf("1. TYPE 1\n");
        printf("2. TYPE 2\n");
        printf("3. TYPE 3\n");
        printf("4. TYPE 4\n");
        printf("5. TYPE 5\n");
        printf("6. TYPE 6\n");
        printf("7. TYPE 7\n");
        printf("0. QUIT\n");
        printf("\nSelect type: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 1 ----------\n\n");
                printf(" ** Show the sales trends for a particular brand over the past k years **\n");
                printf("Which Brand? (ex. Chevrolet, if '0': back to select menu) : ");
                getchar();
                scanf("%[^\n]", brand_name);
                if (strcmp(brand_name, "0") == 0)
                {
                    clrscr();
                    break;
                }
                printf("Which K? ('0': back to select menu) : ");
                scanf("%d", &k);
                if (k == 0)
                {
                    clrscr();
                    break;
                }
                type1(k, brand_name, 1);

                printf("\n---------- Subtypes in TYPE 1 ----------\n");
                printf("1. TYPE 1-1\n");
                printf("\nSelect type ('0': back to select menu) : ");
                scanf("%d", &subChoice);

                while ((subChoice != 0) && (subChoice != 1))
                {
                    printf("\nThere is no type %d\nSelect type again : ", subChoice);
                    scanf("%d", &subChoice);
                }

                switch (subChoice)
                {
                case 1:
                    while (1)
                    {
                        clrscr();
                        printf("\n---------- TYPE 1-1 ----------\n\n");
                        printf("** Then break these data out by gender of the buyer **\n");
                        type1(k, brand_name, 2);

                        printf("\n---------- Subtypes in TYPE 1-1 ----------\n\n");
                        printf("1. TYPE 1-1-1\n");
                        printf("\nSelect type ('0': back to select menu) : ");
                        scanf("%d", &subChoice);

                        while ((subChoice != 0) && (subChoice != 1))
                        {
                            printf("\nThere is no type %d\nSelect type again : ", subChoice);
                            scanf("%d", &subChoice);
                        }

                        switch (subChoice)
                        {
                        case 1:
                            while (1)
                            {
                                clrscr();
                                printf("\n---------- TYPE 1-1-1 ----------\n\n");
                                printf("** Then by income range **\n");
                                type1(k, brand_name, 3);
                                printf("\nPut '0' to go back to select menu: ");
                                scanf("%d", &quit);
                                if (quit == 0)
                                {
                                    clrscr();
                                    break;
                                }
                            }
                            break;

                        case 0:
                            clrscr();
                            quit = 0;
                            break;
                        }
                        if (quit == 0)
                            break;
                    }
                    if (quit == 0)
                        break;
                    break;

                case 0:
                    clrscr();
                    quit = 0;
                    break;
                }
                if (quit == 0)
                    break;
            }
            if (k == 0 || strcmp(brand_name, "0") == 0)
                break;
            break;

        case 2:
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 2 ----------\n\n");
                printf(" ** Show sales trends for various brands over the past k months **\n");
                printf("Which K? ('0': back to select menu) : ");
                scanf("%d", &k);
                if (k == 0)
                {
                    clrscr();
                    break;
                }
                type2(k, 1);

                printf("\n---------- Subtypes in TYPE 2 ----------\n");
                printf("1. TYPE 2-1\n");
                printf("\nSelect type ('0': back to select menu) : ");
                scanf("%d", &subChoice);

                while ((subChoice != 0) && (subChoice != 1))
                {
                    printf("\nThere is no type %d\nSelect type again : ", subChoice);
                    scanf("%d", &subChoice);
                }

                switch (subChoice)
                {
                case 1:
                    while (1)
                    {
                        clrscr();
                        printf("\n---------- TYPE 2-1 ----------\n\n");
                        printf("** Then break these data out by gender of the buyer **\n");
                        type2(k, 2);

                        printf("\n---------- Subtypes in TYPE 2-1 ----------\n\n");
                        printf("1. TYPE 2-1-1\n");
                        printf("\nSelect type ('0': back to select menu) : ");
                        scanf("%d", &subChoice);

                        while ((subChoice != 0) && (subChoice != 1))
                        {
                            printf("\nThere is no type %d\nSelect type again : ", subChoice);
                            scanf("%d", &subChoice);
                        }

                        switch (subChoice)
                        {
                        case 1:
                            while (1)
                            {
                                clrscr();
                                printf("\n---------- TYPE 2-1-1 ----------\n\n");
                                printf("** Then by income range **\n");
                                type2(k, 3);
                                printf("\nPut '0' to go back to select menu: ");
                                scanf("%d", &quit);
                                if (quit == 0)
                                {
                                    clrscr();
                                    break;
                                }
                            }
                            break;

                        case 0:
                            clrscr();
                            quit = 0;
                            break;
                        }
                        if (quit == 0)
                            break;
                    }
                    if (quit == 0)
                        break;
                    break;

                case 0:
                    clrscr();
                    quit = 0;
                    break;
                }
                if (quit == 0)
                    break;
            }
            if (k == 0)
                break;
            break;

        case 3:
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 3 ----------\n\n");
                printf(" ** Find that transmissions made by supplier (company name) between two given dates are defective **\n");
                printf("Which first date(YYYY-MM-DD)? ('0': back to select menu) : ");
                getchar();
                scanf("%s", date1);
                if (strcmp(date1, "0") == 0)
                {
                    clrscr();
                    break;
                }
                while (date1[4] != '-' || date1[7] != '-')
                {
                    printf("Wrong input format: YYYY-MM-DD (ex. 2019-04-01)\nPlease input again : ");
                    getchar();
                    scanf("%s", date1);
                }
                printf("Which second date(YYYY-MM-DD)? ('0': back to select menu) : ");
                getchar();
                scanf("%s", date2);
                if (strcmp(date2, "0") == 0)
                {
                    clrscr();
                    break;
                }
                while (date2[4] != '-' && date2[7] != '-')
                {
                    printf("Wrong input format: YYYY-MM-DD (ex. 2020-04-01)\nPlease input again : ");
                    getchar();
                    scanf("%s", date2);
                }
                printf("Which supplier? (ex. GM Supplier if '0': back to select menu) : ");
                getchar();
                scanf("%[^\n]", supplier_name);
                if (strcmp(supplier_name, "0") == 0)
                {
                    clrscr();
                    break;
                }

                type3(supplier_name, date1, date2, 1);

                printf("\n---------- Subtypes in TYPE 3 ----------\n");
                printf("1. TYPE 3-1\n");
                printf("2. TYPE 3-2\n");
                printf("\nSelect type ('0': back to select menu) : ");
                getchar();
                scanf("%d", &subChoice);

                while ((subChoice != 0) && (subChoice != 1) && (subChoice != 2))
                {
                    printf("\nThere is no type %d\nSelect type again : ", subChoice);
                    getchar();
                    scanf("%d", &subChoice);
                }

                switch (subChoice)
                {
                case 1:
                    while (1)
                    {
                        clrscr();
                        printf("\n---------- TYPE 3-1 ----------\n\n");
                        printf("** Find the VIN of each car containing such a transmission and the customer to which it was sold **\n");
                        type3(supplier_name, date1, date2, 2);

                        printf("\nPut '0' to go back to select menu: ");
                        getchar();
                        scanf("%d", &quit);
                        if (quit == 0)
                        {
                            clrscr();
                            break;
                        }
                    }
                    if (quit == 0)
                        break;
                    break;
                case 2:
                    while (1)
                    {
                        clrscr();
                        printf("\n---------- TYPE 3-2 ----------\n\n");
                        printf("** Find the dealer who sold the VIN and transmission for each vehicle containing these transmissions **\n");
                        type3(supplier_name, date1, date2, 3);

                        printf("\nPut '0' to go back to select menu: ");
                        getchar();
                        scanf("%d", &quit);
                        if (quit == 0)
                        {
                            clrscr();
                            break;
                        }
                    }
                    if (quit == 0)
                        break;
                    break;

                case 0:
                    clrscr();
                    quit = 0;
                    break;
                }
                if (quit == 0)
                    break;
            }
            if (k == 0)
                break;
            break;

        case 4:
            clrscr();
            printf("\n---------- TYPE 4 ----------\n\n");
            printf(" ** Find the top k brands by dollar-amount sold by the year **\n");
            printf("Which year? ('0': back to select menu) : ");
            getchar();
            scanf("%d", &year);
            if (year == 0)
            {
                clrscr();
                break;
            }
            printf("Which K? ('0': back to select menu) : ");
            getchar();
            scanf("%d", &k);
            if (k == 0)
            {
                clrscr();
                break;
            }

            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 4 ----------\n\n");
                printf(" ** Find the top k brands by dollar-amount sold by the year **\n");
                type4(k, year);
                printf("\nPut '0' to go back to select menu: ");
                getchar();
                scanf("%d", &quit);
                if (quit == 0)
                {
                    clrscr();
                    break;
                }
            }
            break;

        case 5:
            clrscr();
            printf("\n---------- TYPE 5 ----------\n\n");
            printf(" ** Find the top k brands by unit sales by the year **\n");
            printf("Which year? ('0': back to select menu) : ");
            getchar();
            scanf("%d", &year);
            if (year == 0)
            {
                clrscr();
                break;
            }
            printf("Which K? ('0': back to select menu) : ");
            getchar();
            scanf("%d", &k);

            if (k == 0)
            {
                clrscr();
                break;
            }
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 5 ----------\n\n");
                printf(" ** Find the top k brands by unit sales by the year **\n");
                type5(k, year);
                printf("\nPut '0' to go back to select menu: ");
                getchar();
                scanf("%d", &quit);
                if (quit == 0)
                {
                    clrscr();
                    break;
                }
            }
            break;

        case 6:
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 6 ----------\n\n");
                printf(" ** In what month(s) do convertibles sell best? **\n");
                type6();

                printf("\nPut '0' to go back to select menu: ");
                getchar();
                scanf("%d", &quit);
                if (quit == 0)
                {
                    clrscr();
                    break;
                }
            }
            break;
        case 7:
            while (1)
            {
                clrscr();
                printf("\n---------- TYPE 7 ----------\n\n");
                printf(" ** Find those dealers who keep a vehicle in inventory for the longest average time **\n");
                type7();

                printf("\nPut '0' to go back to select menu: ");
                getchar();
                scanf("%d", &quit);
                if (quit == 0)
                {
                    clrscr();
                    break;
                }
            }
            break;

        case 0:
            end = 1;
            break;

        default:
            clrscr();
            break;
        }

        if (end == 1)
        {
            break;
        }
    }
}

int drop_tables()
{
    /*
    생성한 테이블의 tuple을 모두 지우고 테이블을 drop 한다.
    */
    char *query;
    int state = 0;
    int i;

    // drop tables
    for (i = 0; i < 2 * table_num + 2; i++)
    {
        // first query: SET FOREIGN_KEY_CHECKS = 0
        // last query: SET FOREIGN_KEY_CHECKS = 1
        query = fgets(line, MAX, fp);
        state = mysql_query(connection, query);
        if (state != 0)
        {
            printf("DROP TABLE ERROR: %s\n", query);
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    MYSQL conn;
    MYSQL_RES *sql_result;
    MYSQL_ROW sql_row;

    if (mysql_init(&conn) == NULL)
        printf("mysql_init() error!");

    connection = mysql_real_connect(&conn, host, user, pw, db, 3306, (const char *)NULL, 0);
    if (connection == NULL)
    {
        printf("%d ERROR: %s\n", mysql_errno(&conn), mysql_error(&conn));
        return 1;
    }

    else
    {
        printf("Connection Succeed\n");

        if (mysql_select_db(&conn, db))
        {
            printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
            return 1;
        }

        fp = fopen("20160768.txt", "r");
        int initialized = initialize_DB();
        if (initialized != 0)
        {
            printf("Initialize DB ERROR\n");
            return 1;
        }

        show_interface();
        drop_tables();

        fclose(fp);
        mysql_close(connection);
    }

    return 0;
}