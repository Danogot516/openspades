/*
 Copyright (c) 2013 yvt
 Portion of the code is based on Serverbrowser.cpp.
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */
#include <OpenSpades.h>
#include "MainScreenHelper.h"
#include "Serverbrowser.h"
#include <Core/AutoLocker.h>
#include <Core/Thread.h>
#include <curl/curl.h>
#include <json/json.h>

#define SERVICE_URL	"http://services.buildandshoot.com/serverlist.json"

namespace spades {
	namespace gui {
		
		// FIXME: mostly duplicated code with Serverbrowser.cpp
		class MainScreenHelper::ServerListQuery: public Thread {
			Mutex infoLock;
			Handle<MainScreenHelper> owner;
			std::string buffer;
			
			static size_t curlWriteCallback( void *ptr, size_t size, size_t nmemb, ServerListQuery* query ) {
				size_t dataSize = size * nmemb;
				return query->writeCallback( ptr, dataSize );
			}
			
			size_t writeCallback( void *ptr, size_t size ) {
				
				buffer.append(reinterpret_cast<char*>(ptr), size);
				return size;
			}
			
			void ReturnResult(MainScreenServerList *list){
				AutoLocker lock(&(owner->newResultArrayLock));
				delete owner->newResult;
				owner->newResult = list;
				owner = NULL; // release owner
			}
			
			void ProcessResponse() {
				Json::Reader reader;
				Json::Value root;
				MainScreenServerList *resp = new MainScreenServerList();
				try{
					if( reader.parse( buffer, root, false ) ) {
						for( Json::Value::iterator it = root.begin(); it != root.end(); ++it ) {
							Json::Value &obj = *it;
							Serveritem* srv = Serveritem::create( obj );
							if( srv ) {
								resp->list.push_back(new MainScreenServerItem(srv));
								delete srv;
							}
						}
					}
					ReturnResult(resp);
				}catch(...){
					delete resp;
					throw;
				}
			}
			
		public:
			
			ServerListQuery(MainScreenHelper *owner):
			owner(owner){
				
			}
			
			virtual void Run() {
				try{
					CURL* cHandle = curl_easy_init();
					if( cHandle ) {
						try{
							curl_easy_setopt( cHandle, CURLOPT_USERAGENT, OpenSpades_VER_STR );
							curl_easy_setopt( cHandle, CURLOPT_URL, SERVICE_URL );
							curl_easy_setopt( cHandle, CURLOPT_WRITEFUNCTION, &ServerListQuery::curlWriteCallback );
							curl_easy_setopt( cHandle, CURLOPT_WRITEDATA, this );
							if( 0 == curl_easy_perform( cHandle ) ) {
								
								ProcessResponse();
								
							} else {
								SPRaise("HTTP request error.");
							}
							curl_easy_cleanup( cHandle );
						}catch(...){
							curl_easy_cleanup( cHandle );
							throw;
						}
					}else{
						SPRaise("Failed to create cURL object.");
					}
				}catch(std::exception& ex) {
					MainScreenServerList *lst = new MainScreenServerList();
					lst->message = ex.what();
					ReturnResult(lst);
				}catch(...) {
					MainScreenServerList *lst = new MainScreenServerList();
					lst->message = "Unknown error.";
					ReturnResult(lst);
				}
			}
		};
		
		MainScreenHelper::MainScreenHelper(MainScreen *scr):
		mainScreen(scr),
		result(NULL),
		newResult(NULL),
		query(NULL){
			SPADES_MARK_FUNCTION();
		}
		
		MainScreenHelper::~MainScreenHelper() {
			SPADES_MARK_FUNCTION();
			if(query) {
				query->MarkForAutoDeletion();
			}
			delete result;
			delete newResult;
		}
		
		void MainScreenHelper::MainScreenDestroyed() {
			SPADES_MARK_FUNCTION();
			mainScreen = NULL;
		}
		
		bool MainScreenHelper::PollServerListState() {
			SPADES_MARK_FUNCTION();
			AutoLocker lock(&newResultArrayLock);
			if(newResult) {
				delete result;
				result = newResult;
				newResult = NULL;
				query->MarkForAutoDeletion();
				query = NULL;
				return true;
			}
			return false;
		}
		
		void MainScreenHelper::StartQuery() {
			if(query) {
				// ongoing
				return;
			}
			
			query = new ServerListQuery(this);
			query->Start();
		}
		
		CScriptArray *MainScreenHelper::GetServerList() {
			if(result == NULL){
				return NULL;
			}
			
			// TODO: sort/filtering
			std::vector<MainScreenServerItem *>& lst = result->list;
			if(lst.empty())
				return NULL;
			
			asIScriptEngine *eng = ScriptManager::GetInstance()->GetEngine();
			asIObjectType* t = eng->GetObjectTypeById(eng->GetTypeIdByDecl("array<spades::MainScreenServerItem@>"));
			SPAssert(t != NULL);
			CScriptArray *arr = new CScriptArray(lst.size(), t);
			for(size_t i = 0; i < lst.size(); i++){
				arr->SetValue((asUINT)i, &(lst[i]));
			}
			return arr;
		}
		
		std::string MainScreenHelper::GetServerListQueryMessage() {
			if(result == NULL)
				return "";
			return result->message;
		}
		
		MainScreenServerList::~MainScreenServerList() {
			for(size_t i = 0; i < list.size(); i++)
				list[i]->Release();
		}
		
		MainScreenServerItem::MainScreenServerItem(Serveritem *item) {
			SPADES_MARK_FUNCTION();
			name = item->Name();
			address = item->Ip();
			mapName = item->Map();
			gameMode = item->GameMode();
			country = item->Country();
			protocol = item->Version();
			ping = item->Ping();
			numPlayers = item->Players();
			maxPlayers = item->MaxPlayers();
		}
		
		MainScreenServerItem::~MainScreenServerItem() {
			SPADES_MARK_FUNCTION();
			
		}
		
		
	}
}
