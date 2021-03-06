/**
 * @file cpp_oper_data_example.cpp
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief Example application that uses sysrepo as the configuraton datastore.
 *
 * @copyright
 * Copyright 2019 Deutsche Telekom AG.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "Session.hpp"

#define MAX_LEN 100

using namespace std;

volatile int exit_application = 0;

class My_Callback1:public sysrepo::Callback {
    /* Function to be called whenever a client requests these state data. */
    int oper_get_items(sysrepo::S_Session session, const char *module_name, const char *path, const char *request_xpath, \
            uint32_t request_id, libyang::S_Data_Node &parent, void *private_data) override
    {
        cout << "\n\n ========== CALLBACK CALLED TO PROVIDE \"" << path << "\" DATA ==========\n" << endl;

        libyang::S_Context ctx = session->get_context();
        libyang::S_Module mod = ctx->get_module(module_name);

        parent.reset(new libyang::Data_Node(ctx, "/ietf-interfaces:interfaces-state", nullptr, LYD_ANYDATA_CONSTSTRING, 0));

        libyang::S_Data_Node ifc(new libyang::Data_Node(parent, mod, "interface"));
        libyang::S_Data_Node name(new libyang::Data_Node(ifc, mod, "name", "eth100"));
        libyang::S_Data_Node type(new libyang::Data_Node(ifc, mod, "type", "iana-if-type:ethernetCsmacd"));
        libyang::S_Data_Node oper_status(new libyang::Data_Node(ifc, mod, "oper-status", "down"));

        ifc.reset(new libyang::Data_Node(parent, mod, "interface"));
        name.reset(new libyang::Data_Node(ifc, mod, "name", "eth101"));
        type.reset(new libyang::Data_Node(ifc, mod, "type", "iana-if-type:ethernetCsmacd"));
        oper_status.reset(new libyang::Data_Node(ifc, mod, "oper-status", "up"));

        ifc.reset(new libyang::Data_Node(parent, mod, "interface"));
        name.reset(new libyang::Data_Node(ifc, mod, "name", "eth102"));
        type.reset(new libyang::Data_Node(ifc, mod, "type", "iana-if-type:ethernetCsmacd"));
        oper_status.reset(new libyang::Data_Node(ifc, mod, "oper-status", "dormant"));

        ifc.reset(new libyang::Data_Node(parent, mod, "interface"));
        name.reset(new libyang::Data_Node(ifc, mod, "name", "eth105"));
        type.reset(new libyang::Data_Node(ifc, mod, "type", "iana-if-type:ethernetCsmacd"));
        oper_status.reset(new libyang::Data_Node(ifc, mod, "oper-status", "not-present"));

        return SR_ERR_OK;
    }
};

class My_Callback2:public sysrepo::Callback {
    /* Function to be called whenever a client requests these state data. */
    int oper_get_items(sysrepo::S_Session session, const char *module_name, const char *path, const char *request_xpath, \
            uint32_t request_id, libyang::S_Data_Node &parent, void *private_data) override
    {
        cout << "\n\n ========== CALLBACK CALLED TO PROVIDE \"" << path << "\" DATA ==========\n" << endl;

        libyang::S_Context ctx = session->get_context();
        libyang::S_Module mod = ctx->get_module(module_name);

        libyang::S_Data_Node stats(new libyang::Data_Node(parent, mod, "statistics"));
        libyang::S_Data_Node dis_time(new libyang::Data_Node(stats, mod, "discontinuity-time", "2019-01-01T00:00:00Z"));
        libyang::S_Data_Node in_oct(new libyang::Data_Node(stats, mod, "in-octets", "22"));

        return SR_ERR_OK;
    }
};

static void
sigint_handler(int signum)
{
    exit_application = 1;
}

/* Notable difference between c implementation is using exception mechanism for open handling unexpected events.
 * Here it is useful because `Conenction`, `Session` and `Subscribe` could throw an exception. */
int
main(int argc, char **argv)
{
    const char *module_name = "ietf-interfaces";
    try {
        cout << "Application will provide data of " << module_name << endl;
        sysrepo::S_Connection conn(new sysrepo::Connection());
        sysrepo::S_Session sess(new sysrepo::Session(conn));

        sysrepo::S_Subscribe subscribe(new sysrepo::Subscribe(sess));
        sysrepo::S_Callback cb1(new My_Callback1());
        sysrepo::S_Callback cb2(new My_Callback2());

        subscribe->oper_get_items_subscribe(module_name, "/ietf-interfaces:interfaces-state", cb1);
        subscribe->oper_get_items_subscribe(module_name, "/ietf-interfaces:interfaces-state/interface/statistics", cb2,
                nullptr, SR_SUBSCR_CTX_REUSE);

        /* loop until ctrl-c is pressed / SIGINT is received */
        signal(SIGINT, sigint_handler);
        while (!exit_application) {
            sleep(1000);  /* or do some more useful work... */
        }

        cout << "Application exit requested, exiting." << endl;

    } catch( const std::exception& e ) {
        cout << e.what() << endl;
        return -1;
    }

    return 0;
}
