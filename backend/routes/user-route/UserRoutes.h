//
// Created by themi on 3/8/26.
//

#ifndef RYKLYS_BACKEND_USERROUTES_H
#define RYKLYS_BACKEND_USERROUTES_H
#include "../../zvejys-rest-api/ZvejysServer.h"
#include "../../pg-connection-pool/pgPool.h"
#include "./../../database/database_repositories/UserRepository.h"

void RegisterUserRoutes(ZvejysServer &server);



#endif //RYKLYS_BACKEND_USERROUTES_H
