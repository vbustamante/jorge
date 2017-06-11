#include <jorgeLua.h>

#include <sqlite3.h>

#include <stdlib.h>
#include <string.h>


struct jlua_response_body_node{
  char *data;
  struct jlua_response_body_node *next;
};

struct jlua_global_data{
  int connection;
  jlua_response_body_node *response_body;
  int response_body_length;
  char *response_header;
};

// Creates the default folder and hello world lua script, if not existing
// Also sets up the database.
void jlua_setup_environment(){
  
  { // Lua scripts
    int mkdirStatus = mkdir(JLUA_SCRIPT_PATH, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
    if (mkdirStatus != -1){
      printf("Assuming setup.\nCreating jorgeScripts folder and index.lua.\n");
      
      FILE *helloWorldFile = fopen (JLUA_SCRIPT_PATH"/index.lua","w");
      fputs (JLUA_HELLOW_TEXT, helloWorldFile); // Saves data defined on jorgeLua.h
      fclose (helloWorldFile);
      
      FILE *notFoundFile = fopen (JLUA_SCRIPT_PATH"/404.lua","w");
      fputs (JLUA_404_TEXT, notFoundFile); // Saves data defined on jorgeLua.h
      fclose (notFoundFile);
      
      // TODO try to bake in lua scripts from standalone files at compile time.
      // Cmake possibly could help. 
    }
  }
  
  { // Setup database
  
    sqlite3 *db;
    int dbErr = sqlite3_open("jorge.db", &db);

    if(dbErr){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return;
    }
    
    fprintf(stderr, "Opened database successfully\n");
    
    sqlite3_stmt *statement;
    
    sqlite3_prepare_v2(db, 
      "CREATE TABLE messages("
        "id     INTEGER PRIMARY KEY,"
        "msg    TEXT NOT NULL,"
        "sender TEXT,"
        "time   TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
      ")", 
      -1, &statement, NULL);
    
    int result = sqlite3_step(statement);
    
    if(result == SQLITE_DONE){
      printf("Setting up tables\n");
    }
    
    sqlite3_finalize(statement);
    
    sqlite3_close(db);
  }
  
  
  printf("Welcome to jorge!\n");
}

// This global is the way jluaf functions can interact with request data
// This should be only accessible inside the jLua module.
struct jlua_global_data jData;


// The entry point to the lua module. Gets a connection and request data,
// starts a lua interpreter and runs the related scripts
// TODO Select script to run based on the request path
void jlua_interpret(int conn_fd, struct jnet_request_data request){
  
  // Setup connection data
  jData.connection = conn_fd;
  jData.response_body_length = 0;
  jData.response_body = NULL;
  jData.response_header = NULL;
  
  // Deal with request data
  
  // Fire up interpreter
  int luaStatus;
  lua_State *L = luaL_newstate();
  
  // Init functions
  luaL_openlibs(L);
  lua_register(L, "echo", jluaf_echo);
  lua_register(L, "setHeader", jluaf_setHeader);
  lua_register(L, "sqlQuery", jluaf_sqlQuery);

  // Load Script
  {

    int lastChar = 0;
    while(request.path[lastChar] != '\0') lastChar++;
    
    // This here is a REALLY HACKY solution
    // Since the line which contains the path ends with  ` HTTP/1.X\r\n`
    // and the version is saved as value, we assume that `index.lua\0` 
    // will fit there without messing up the next line.
    // Coincidence? sure. but it works.
    if(request.path[lastChar-1] == '/'){
      request.path[lastChar++] = 'i';
      request.path[lastChar++] = 'n';
      request.path[lastChar++] = 'd';
      request.path[lastChar++] = 'e';
      request.path[lastChar++] = 'x';
      request.path[lastChar++] = '.';
      request.path[lastChar++] = 'l';
      request.path[lastChar++] = 'u';
      request.path[lastChar++] = 'a';
      request.path[lastChar]   = '\0';
    }
    

    size_t  scriptPathLength = strlen(JLUA_SCRIPT_PATH) + strlen(request.path) + 1;
    char *  scriptPath = (char *) malloc(scriptPathLength * sizeof(*scriptPath)); 
    
    memset(scriptPath, '\0', scriptPathLength);
    
    strcat(scriptPath, JLUA_SCRIPT_PATH);
    strcat(scriptPath, request.path);
    
    luaStatus = luaL_loadfile(L, scriptPath);
    free(scriptPath);
    if(luaStatus){
      jlua_print_error(L);
      luaStatus = luaL_loadfile(L, JLUA_SCRIPT_PATH"/404.lua");
      if(luaStatus){
        jlua_print_error(L);
        return;
      }
    }
  }
  
  luaStatus = lua_pcall(L, 0, 0, 0);
  bool scriptError = false;
  if(luaStatus){
    // Todo HTTP error on script error
    jlua_print_error(L);
    scriptError = true;
  }


  // Send Header
  size_t headerLen = strlen(jData.response_header);
  if(!scriptError) jnet_send_all(jData.connection, jData.response_header, &headerLen, 0);
  free(jData.response_header);

  // Send body
  jlua_response_body_node *walker = jData.response_body;
  jlua_response_body_node *last;

  while(walker != NULL){
    size_t bodyLen = strlen(walker->data);
    if(!scriptError) jnet_send_all(jData.connection, walker->data, &bodyLen, !walker->next);
    
    last    = walker;
    walker  = walker->next;  

    free(last->data);

    free(last);
  }

  lua_close(L);
}

// Prints a lua error.
// The existance of an error is indicated by a lua state function returning a false value
void jlua_print_error(lua_State* L) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char* message = lua_tostring(L, -1);
  fprintf(stderr, "%s\n", message);
  lua_pop(L, 1);
}

// All jluaf functions are made to be registered and used in lua

// Receives any number of strings and adds them to the body linked list
// The list will be unrolled, sent and freed after the headers are sent
int jluaf_echo(lua_State* L){
  
  int args = lua_gettop(L);

  for(int i=1; i<=args; i++){
    
    struct jlua_response_body_node *response_body_end;
    
    response_body_end = 
      (struct jlua_response_body_node*) malloc(sizeof(*response_body_end)+1);
    
    response_body_end->next = NULL;
    response_body_end->data = strdup(lua_tostring(L, i));
    size_t body_bytes = strlen(response_body_end->data);
    jData.response_body_length += body_bytes;

    lua_pushnumber(L, body_bytes);
    // TODO maybe checkout and correct linebreaks
    // Though this is easier to be done on the scripts
    // Maybe we could create a lua function that abstracts this callable
    
    if(jData.response_body){
      struct jlua_response_body_node *walker = jData.response_body;
      
      while(walker->next != NULL) walker = walker->next;
      
      walker->next = response_body_end;
    }else jData.response_body = response_body_end;
  }
  
  return 1;
}

// Adds a single string to the headers linked list
// The list will be unrolled, sent and freed at the end of the request processing
int jluaf_setHeader(lua_State* L){

  if(jData.response_header) free(jData.response_header);
  jData.response_header = strdup(lua_tostring(L, 1));

  lua_pushnumber(L, strlen(jData.response_header));
  return 1;
}

int jluaf_sqlQuery(lua_State *L){
  
  int args = lua_gettop(L);
  if(args < 1 || lua_type(L, -1) != LUA_TSTRING){
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "count");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "modified");
    lua_newtable(L);
    lua_setfield(L, -2, "data");
    lua_pushstring(L, "Missing query");
    return 2;
  }
 
  char *queryString = strdup(lua_tostring(L, -1));
  
  
  sqlite3 *db;
  int dbErr = sqlite3_open("jorge.db", &db);

  if(dbErr){
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "count");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "modified");
    lua_newtable(L);
    lua_setfield(L, -2, "data");
    lua_pushstring(L, sqlite3_errmsg(db));
    return 2;
  }
 
  sqlite3_stmt *statement;
 
  sqlite3_prepare_v2(db, queryString, -1, &statement, NULL);
      
  int result = sqlite3_step(statement);
  int modRows= sqlite3_changes(db);
  
  
  lua_newtable(L);
  lua_newtable(L);
  char *err = NULL;
  int rows = 0;
  while(result != SQLITE_DONE){

    if(result != SQLITE_ROW){
      err = strdup(sqlite3_errmsg(db));
      break;
    } 

    rows++;
    int colCount = sqlite3_column_count(statement);
    
    lua_pushnumber(L, rows);
    lua_createtable(L, 0, colCount);
    
    for(int i=0; i < colCount; i++){

      printf(" %s is ", sqlite3_column_name(statement, i));
      switch(sqlite3_column_type(statement, i)){
        case SQLITE_INTEGER:
          lua_pushinteger(L, sqlite3_column_int(statement, i));
          
          printf("int: %d|", sqlite3_column_int(statement, i));
          break;
        case SQLITE_FLOAT:
          lua_pushnumber(L, sqlite3_column_double(statement, i));
           
          printf("float: %.2f|", sqlite3_column_double(statement, i));
          break;
        case SQLITE_TEXT:
          lua_pushstring(L, (const char*)sqlite3_column_text(statement, i));
          
          printf("text: %s|", (const char*)sqlite3_column_text(statement, i));
          
          break;
        case SQLITE_BLOB:
          lua_pushnil(L);
          printf("BLOB|");
          break;
        case SQLITE_NULL:
          lua_pushnil(L);
          printf("NULL|");
          break;
      }
      
      lua_setfield(L, -2, sqlite3_column_name(statement, i));
    }
    lua_settable(L, -3);
    
    result = sqlite3_step(statement);
    printf("\n");
  }
  
  lua_setfield(L, -2, "data");
  
  lua_pushinteger(L, rows);
  lua_setfield(L, -2, "count");
  
  lua_pushinteger(L, modRows);
  lua_setfield(L, -2, "modified");


  if(err){
    lua_pushstring(L, err);
    free(err);
  }else{
    lua_pushnil(L);
  }
  

  sqlite3_finalize(statement);
  sqlite3_close(db); 
  free(queryString);
  
  return 2;
}
