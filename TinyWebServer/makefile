server: main.cpp ./log/log.h ./log/log.cpp ./log/block_queue.h ./timer/lst_timer.h ./http/http_conn.cpp ./http/http_conn.h ./threadpool/threadpool.h ./lock/locker.h  ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h
	g++ -o server main.cpp ./log/log.h ./log/log.cpp ./log/block_queue.h ./timer/lst_timer.h ./http/http_conn.cpp ./http/http_conn.h ./threadpool/threadpool.h ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h -lpthread -lmysqlclient


clean:
	rm  -r server