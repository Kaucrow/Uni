#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <fcntl.h>              // For _setmode().
#include <libloaderapi.h>
#include <sys/stat.h>
#include <boost/locale.hpp>
#include <structs.h>
#include <merge_sort.h>
#include <store_movie.h>
#include <rent_movie.h>
#include <search_ops.h>
#include <file_ops.h>

using   std::wcout, std::wcerr, std::wcin, std::getline, std::wfstream,
        std::wifstream, std::wofstream, std::wstring;

enum {  FILTER = 1, GETMOVDATA = 2, ADD = 3, RENT = 4,      // Actions.
        RETURNMOV = 5, GETCLTDATA = 6, EXIT = 7 };
enum { DUR, PRC, YEA, MON, DAY };           // int frag types.
enum { TTL, DIR };                          // wstring frag types.

/**
 * @brief OS agnostic clear screen function.
 */
void ClrScr();

/**
 * @brief Gets the path to the "data" dir.
 * @return The path to the data dir, if found. Otherwise, returns an empty wstring.
*/
std::string GetDataDir();

 /**
 * @brief Gets the current date, or the current date plus 14 days.
 * @param add14Days - Depending on its value, 14 days are added to the current date or not.
 * @return Returns a wstring object with the date in "Y-M-d" format.
 */
wstring GetDate(bool add14Days = false);

int main(){
    boost::locale::generator gen;               // Create the locale generator.
    std::locale loc = gen("en_US");             // Create an "en_US" locale
    std::locale::global(loc);                   // and set it as the global locale.
    _setmode(_fileno(stdout), _O_U8TEXT);       // Change the STDOUT mode to use UTF-8 characters.

    // Set the file paths. //
    std::string dataDir = GetDataDir();

    if(dataDir == ""){ wcerr << L"[ ERR ] THE PROGRAM COULD NOT FIND THE \"data\" DIR.\n"; return 1; }

    char USRDATA_PATH[dataDir.length() + 16];
    strcpy(USRDATA_PATH, (dataDir + "\\users_data.csv").c_str());

    char MOVFILE_PATH[dataDir.length() + 12];
    strcpy(MOVFILE_PATH, (dataDir + "\\movies.csv").c_str());

    char TTLFILE_PATH[dataDir.length() + 11];
    strcpy(TTLFILE_PATH, (dataDir + "\\title.txt").c_str());
    
    // Ensure that the movies.csv file has all the required fields. //
    CheckMoviesCsv(MOVFILE_PATH);

    int totalMovies = 0;
    try{ totalMovies = GetLastLineFirstNum(MOVFILE_PATH); }     // Get the number of movies in the movies.csv file.
    catch(wstring exc){ wcerr << exc << '\n'; return 1; }

    Movie baseList[totalMovies + 3001];     // Create a base list of movies (sorted by ID), where the first element is unused.
    // ==================
    // Create lists of movie fragments, where the first element of the array is unused.
    // Each frag list is sorted according to the movie data it contains 
    // (duration, title, director, release year, release month, release day).
    // ==================
    IntFrag durList[totalMovies + 3001];
    IntFrag prcList[totalMovies + 3001];
    IntFrag yeaList[totalMovies + 3001];
    IntFrag monList[totalMovies + 3001];
    IntFrag dayList[totalMovies + 3001];
    WstrFrag ttlList[totalMovies + 3001];
    WstrFrag dirList[totalMovies + 3001];
    IntFrag* intFrags[5] = { durList, prcList, yeaList, monList, dayList };
    WstrFrag* wstrFrags[2] = { ttlList, dirList };

    wstring username;           // Username of the active user.
    int userNum = 0;            // Number of users in the users_data.csv file.
    
    // Display the program title. //
    ClrScr();
    {
        wstring readingLine;
        wifstream titleFile(TTLFILE_PATH);
        if(!titleFile){ wcerr << L"[ ERR ] COULD NOT OPEN TITLE FILE.\n"; return 1; }
        while(getline(titleFile, readingLine)){
            wcout << readingLine << '\n';
        }
    }
    wcin.get();
    
    // Check if the users_data.csv file exists. If it doesn't, output a prompt to create it. //
    {
        wfstream openTest(USRDATA_PATH);
        if(!openTest){
            wstring appendLine;
            ClrScr();
            wcout   << "[ INFO ] No users_data.csv file was found. Please input the name\n"
                    << "         of the first user, so the file may be created: ";
            getline(wcin, username);
            appendLine = L"1;" + username + L";\n";
            AppendLine(USRDATA_PATH, L"id;name;movies\n" + appendLine);
        }
    }

    try{ userNum = GetLastLineFirstNum(USRDATA_PATH); }
    catch(wstring exc){ wcerr << exc << '\n'; return 1; }

    User userList[userNum + 101];       // List which holds the users in the users_data.csv file and their data.
    WstrFrag usernameList[userNum + 101];

    // Populate the user list. //
    try{ PopulateUserList(userList, USRDATA_PATH); }
    catch(wstring exc){ wcerr << exc << '\n'; return 1; }

    // Populate the base movie list. //
    try{ PopulateMovieList(baseList, MOVFILE_PATH); }
    catch(wstring exc){ wcerr << exc << '\n'; return 1; }

    // Copy the base movie list elements data to each frag list. //
    for(int i = 1; i <= totalMovies; i++){
        for(int j = DUR; j <= DAY; j++){ intFrags[j][i].ID = baseList[i].ID; }
        for(int j = TTL; j <= DIR ; j++){ wstrFrags[j][i].ID = baseList[i].ID; }
        intFrags[DUR][i].data = baseList[i].duration;
        intFrags[PRC][i].data = baseList[i].price;
        intFrags[YEA][i].data = baseList[i].release.year;
        intFrags[MON][i].data = baseList[i].release.month;
        intFrags[DAY][i].data = baseList[i].release.day;
        wstrFrags[TTL][i].data = baseList[i].title;
        wstrFrags[DIR][i].data = baseList[i].director;
    }
 
    // Sort each frag list. //
    for(int i = 0; i < 4; i++)
        MergeSort(intFrags[i], 1, totalMovies);
    for(int i = 0; i < 2; i++)
        MergeSort(wstrFrags[i], 1, totalMovies);

    // Copy the userList elements to the username frag list. //
    for(int i = 1; i <= userNum; i++){
        usernameList[i].ID = userList[i].ID;
        usernameList[i].data = userList[i].name;
    }

    // Sort the username frag list. //
    MergeSort(usernameList, 1, userNum);

    // =========================
    //  Main loop.
    // =========================
    int action;
    int currUser;       // ID of the active user.
    while(true){
        // Main menu screen. //
        ClrScr();
        wcout   << "*** MENU ***\n"
                << "(1) Login\n"
                << "(2) Exit\n"
                << "Select option: ";
        wcin >> action;
        switch(action){
            case 1: break;
            case 2:
                wcout << "\nTerminating execution...\n";
                return 0;
        }
        wcin.ignore(1);

        // Login screen. //
        ClrScr(); 
        currUser = 0;
        wcout   << "*** LOGIN ***\n"
                << "-> User: ";
        getline(wcin, username);

        // Check if the user is already in the user list. If it is, set the currUser ID accordingly.
        // If it's not, add it, and set the currUser ID.
        for(int i = 1; i <= userNum; i++){
            if(userList[i].name == username){
                currUser = userList[i].ID;
            }
        }
        if(currUser == 0){
            userNum++;              // Increment the user count. //

            // Update the users_data.csv and the live users data with the new user's data. //
            AppendLine(USRDATA_PATH, std::to_wstring(userNum) + L';' + username + L";\n");
            userList[userNum].ID = userNum;
            userList[userNum].name = username;

            currUser = userNum;     // Set the currUser. //
        }

        while(true){
            // Main actions screen. //
            ClrScr();
            wcout   << "*** CHOOSE AN ACTION ***\n"
                    << "(1) Search with filters\n"
                    << "(2) Get movie info\n"
                    << "(3) Add a movie\n"
                    << "(4) Rent a movie\n"
                    << "(5) Return a movie\n"
                    << "(6) Get client info\n"
                    << "(7) Exit\n"
                    << "\nActive user: " << username << "\n\n"
                    << "Select option: ";
            wcin >> action;
            while(action < FILTER || action > EXIT){
                wcout << "INVALID OPTION.\nSelect option: ";
                wcin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                wcin >> action;
            }
            wcin.ignore(1);

            //=========================
            //  Search with filters.
            // ========================
            if(action == FILTER){
                ClrScr();
                int idMatches[totalMovies + 3001];      // Movie list array to stores the search matches.

                wcout   << "*** FILTERS ***\n"
                        << "(1) Duration\n"
                        << "(2) Title\n"
                        << "(3) Genre\n"
                        << "(4) Director\n"
                        << "(5) Price\n"
                        << "(6) Rease year\n"
                        << "(7) Release month\n"
                        << "(8) Release day\n"
                        << "Select option: ";

                wcin >> action;
                while(action < 1 || action > 8){
                    wcout << "INVALID OPTION.\nSelect option: ";
                    wcin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    wcin >> action;
                }
                wcin.ignore(1);

                wstring wstrSearch;
                int intSearch;
                // ========================
                // Generate the "toMatch" movie according to the selected filter,
                // perform the search, and store the ID of the matching movies in the "idMatches" array.
                // ========================
                ClrScr();
                switch(action){
                    case 1:
                        wcout << "Duration to search for: ";
                        wcin >> intSearch;
                        wcin.ignore(1);
                        BinSearchStoreMatches(intFrags[DUR], idMatches, 1, totalMovies, intSearch);
                        break;
                    case 2:
                        wcout << "Title to search for: ";
                        getline(wcin, wstrSearch);
                        BinSearchStoreMatches(wstrFrags[TTL], idMatches, 1, totalMovies, wstrSearch);
                        break;
                    case 3:
                        wcout << "Genre to search for: ";
                        getline(wcin, wstrSearch);
                        GenreSearchStoreMatches(baseList, idMatches, 1, totalMovies, wstrSearch);
                        break;
                    case 4:
                        wcout << "Director to search for: ";
                        getline(wcin, wstrSearch);
                        BinSearchStoreMatches(wstrFrags[DIR], idMatches, 1, totalMovies, wstrSearch);
                        break;
                    case 5:
                        wcout << "Max price: ";
                        wcin >> intSearch;
                        wcin.ignore(1);
                        BinSearchStoreMatches(intFrags[PRC], idMatches, 1, totalMovies, intSearch, true);
                        break;
                    case 6:
                        wcout << "Year to search for: ";
                        wcin >> intSearch;
                        wcin.ignore(1);
                        BinSearchStoreMatches(intFrags[YEA], idMatches, 1, totalMovies, intSearch);
                        break;
                    case 7:
                        wcout << "Month to search for: ";
                        wcin >> intSearch;
                        while(intSearch < 1 || intSearch > 12){
                            wcout << "Please input a valid month.\n";
                            wcin >> intSearch;
                        }
                        wcin.ignore(1);
                        BinSearchStoreMatches(intFrags[MON], idMatches, 1, totalMovies, intSearch);
                        break;
                    case 8:
                        wcout << "Day to search for: ";
                        wcin >> intSearch;
                        while(intSearch < 1 || intSearch > 31){
                            wcout << "Please input a valid day.\n";
                            wcin >> intSearch;
                        }
                        wcin.ignore(1);
                        BinSearchStoreMatches(intFrags[DAY], idMatches, 1, totalMovies, intSearch);
                        break;
                }

                ClrScr();
                // If no matches were found, output a message. //
                if(idMatches[0] == 0){
                    wcout << L"*** FOUND NO MATCHES ***\n";
                } 
                // If matches were found, print the matching movies from the "idMatches" array. //
                else{
                    wcout << L"*** FOUND MATCHES ***\n";
                    for(int i = 0; idMatches[i] != 0; i++){
                        wcout << baseList[idMatches[i]].title << '\n';
                    }
                }
                wcin.get();
            }
            // ========================
            //  Get movie info.
            // ========================
            else if(action == GETMOVDATA){
                ClrScr();
                wstring search;
                wcout << "-> Movie title: ";
                getline(wcin, search);

                // Perform a search by title for the entered movie. //
                int ttlPos = BinSearch(wstrFrags[TTL], 1, totalMovies, search);

                // If it doesn't exist in the title frag list, output an error and go to the next loop iteration. //
                if(ttlPos == -1){
                    wcerr << L"[ ERR ] THE MOVIE DOES NOT EXIST.\n";
                    wcin.get();
                    continue;
                }

                // If it exists, get its ID, and print its data from the base list. //
                int moviePos = wstrFrags[TTL][ttlPos].ID;
                wcout   << L"\n[ INFO ] Found movie \"" << search << L"\".\n"
                        << L"-> Title: " << baseList[moviePos].title << L'\n'
                        << L"-> Duration: " << baseList[moviePos].duration << L" min.\n"
                        << L"-> Director: " << baseList[moviePos].director << L'\n'
                        << L"-> Price: " << baseList[moviePos].price << L"$\n"
                        << L"-> Release date: " << baseList[moviePos].release.year << L'-' << baseList[moviePos].release.month << L'-' << baseList[moviePos].release.day << L'\n'
                        << L"-> Genres:\n";
                for(int i = 0; baseList[moviePos].genres[i] != L""; i++)
                    wcout << L"  * " << baseList[moviePos].genres[i] << L'\n';

                // The rent data is only printed if the movie has actually been rented to someone. //
                if(baseList[moviePos].status != MOV_STATUS_RETURNED){
                    wcout   << L"-> Rented to: " << baseList[moviePos].rentedTo << L'\n'
                            << L"-> Rented on: " << baseList[moviePos].rentedOn.year << L'-' << baseList[moviePos].rentedOn.month << L'-' << baseList[moviePos].rentedOn.day << L'\n'
                            << L"-> Expiry: " << baseList[moviePos].expiry.year << L'-' << baseList[moviePos].expiry.month << L'-' << baseList[moviePos].expiry.day << L'\n';
                }
                wcin.get();
            }
            // ========================
            //  Add a movie.
            // ========================
            else if(action == ADD){
                ClrScr();
                Movie toStore;
                wstring storeDat;
                totalMovies++;              // Increase the count of movies.
                toStore.ID = totalMovies;

                wcout << "*** NEW MOVIE DATA ***\n";

                /* Get each of the 6 basic data fields, and update the toStore movie accordingly. */
                wcout << "-> Duration: ";
                getline(wcin, storeDat);
                while(stoi(storeDat) <= 0){
                    wcerr << "[ ERR ] The movie cannot have a duration less than or equal to 0.\n-> Duration:";
                    getline(wcin, storeDat);
                }
                toStore.duration = stoi(storeDat);

                wcout << "-> Title: ";
                getline(wcin, storeDat);
                storeDat[0] = toupper(storeDat[0]);
                toStore.title = storeDat;

                wcout << "-> Genres\n";
                for(int i = 0; i < 7; i++){
                    wcout << "   * " << i + 1 << ": ";
                    getline(wcin, storeDat);
                    toStore.genres[i] = storeDat;
                }

                wcout << "-> Director: ";
                getline(wcin, storeDat);
                toStore.director = storeDat;
                
                wcout << "-> Price: ";
                getline(wcin, storeDat);
                toStore.price = stof(storeDat);

                wcout << "-> Release year: ";
                getline(wcin, storeDat);
                toStore.release.year = stoi(storeDat);
                
                wcout << "-> Release month: ";
                getline(wcin, storeDat);
                toStore.release.month = stoi(storeDat);
                
                wcout << "-> Release day: ";
                getline(wcin, storeDat);
                toStore.release.day = stoi(storeDat);
                
                // Store the movie in the baseList. //
                baseList[totalMovies] = toStore;
                // ======================
                // Update each of the frag lists with the added movie data,
                // while preserving the sorting order in each of them.
                // ======================
                StoreNewFrag(intFrags[DUR], 1, totalMovies, toStore.duration);
                StoreNewFrag(intFrags[PRC], 1, totalMovies, int(toStore.price));
                StoreNewFrag(intFrags[YEA], 1, totalMovies, toStore.release.year);
                StoreNewFrag(intFrags[MON], 1, totalMovies, toStore.release.month);
                StoreNewFrag(intFrags[DAY], 1, totalMovies, toStore.release.day);
                StoreNewFrag(wstrFrags[TTL], 1, totalMovies, toStore.title);
                StoreNewFrag(wstrFrags[DIR], 1, totalMovies, toStore.director);

                // Build the line containing the added movie data, and append it to the movies.csv file. //
                std::wostringstream appendLine;
                appendLine << totalMovies << L';' << toStore.title << L';';
                for(int i = 0; toStore.genres[i] != L"" && i < 7; i++){
                    appendLine << toStore.genres[i] << L'|';
                }
                appendLine.seekp(-1, std::ios_base::end);
                appendLine  << L';' << toStore.duration << L';'
                            << toStore.director << L';'
                            << std::setprecision(4) << toStore.price << L';'
                            << std::setfill(L'0') << std::setw(4) << toStore.release.year << L'-'
                            << std::setfill(L'0') << std::setw(2) << toStore.release.month << L'-'
                            << std::setfill(L'0') << std::setw(2) << toStore.release.day << L";;;;\n";

                AppendLine(MOVFILE_PATH, appendLine.str());

                wcout << "[ INFO ] The movie was added successfully.\n";
                wcin.get();
            }
            // ========================
            //  Rent a movie.
            // ========================
            else if(action == RENT){
                ClrScr();
                wstring rentName;
                wcout << "*** MOVIE RENT ***\n";
                wcout << "Input the title of the movie: ";
                getline(wcin, rentName);

                int queryMovieID = 0; 
                // Search for the title of the movie to rent, throw an error if it doesn't exist,
                // and print some information if the movie exists but is already rented.
                int rentStatus = QueryMovieRent(baseList, wstrFrags[TTL], totalMovies, rentName, queryMovieID);
                if(rentStatus == QUERY_RENT_NOTFOUND){
                    wcerr << L"[ ERR ] THE MOVIE DOES NOT EXIST.\n";
                    wcin.get();
                    continue;
                } 
                else if(rentStatus == QUERY_RENT_RENTED){
                    wcout << L"[ INFO ] The movie is already rented by someone.\n";
                    wcin.get();
                    continue;
                }

                // If the movie exists and is not rented, rent it. //
                wstring currDate = GetDate();           // Get the current date.
                wstring expiryDate = GetDate(true);     // Get the expiry date.

                // Update the movies.csv file with the rent information. //
                UpdateMoviesCsv(MOVFILE_PATH, queryMovieID, username, currDate, expiryDate, UPDATE_RENT);
                // Update the base list movie data with the rent information. //
                UpdateMovieLiveData(baseList, queryMovieID, username, currDate, expiryDate, UPDATE_RENT);
                // Update the users_data.csv file and user list data with the rent information. //
                UpdateUsersData(USRDATA_PATH, userList, currUser, rentName, UPDATE_RENT);
            }
            // ========================
            //  Return a movie.
            // ========================
            else if(action == RETURNMOV){
                ClrScr();
                int queryMovieID = 0;
                wstring returnName;
                wcout << "*** MOVIE RETURN ***\n";
                wcout << "Input the title of the movie: ";
                getline(wcin, returnName);

                int rentStatus = QueryMovieRent(baseList, wstrFrags[TTL], totalMovies, returnName, queryMovieID);
                if(rentStatus == QUERY_RENT_NOTFOUND){
                    wcout << L"[ ERR ] THE MOVIE DOES NOT EXIST.\n";
                    wcin.get();
                    continue;
                }
                else if(rentStatus == QUERY_RENT_NOTRENTED){
                    wcout << L"[ INFO ] The movie has not been rented.\n";
                    wcin.get();
                    continue;
                }
                else if(baseList[queryMovieID].rentedTo != username){
                    wcout << L"[ INFO ] You are not the owner of the rent.\n";
                    wcin.get();
                    continue;
                }

                wstring rentDate =
                std::to_wstring(baseList[queryMovieID].rentedOn.year) + L"-" +
                std::to_wstring(baseList[queryMovieID].rentedOn.month) + L"-" +
                std::to_wstring(baseList[queryMovieID].rentedOn.day);

                wstring expiryDate =
                std::to_wstring(baseList[queryMovieID].expiry.year) + L"-" +
                std::to_wstring(baseList[queryMovieID].expiry.month) + L"-" +
                std::to_wstring(baseList[queryMovieID].expiry.day);

                // Update the movies.csv file with the return information. //
                UpdateMoviesCsv(MOVFILE_PATH, queryMovieID, username, rentDate, expiryDate, UPDATE_RETURN);
                // Update the base list movie data with the return information. //
                UpdateMovieLiveData(baseList, queryMovieID, username, rentDate, expiryDate, UPDATE_RETURN);
                // Update the users_data.csv file and user list data with the return information. //
                UpdateUsersData(USRDATA_PATH, userList, currUser, returnName, UPDATE_RETURN);
            }
            else if(action == GETCLTDATA){
                ClrScr();
                wstring search;

                wcout << "Client name: ";
                getline(wcin, search);

                int userPos = BinSearch(usernameList, 1, userNum, search);

                if(userPos == -1)
                    wcerr << "[ ERR ] The client was not found.\n";
                else{
                    ClrScr();
                    userPos = usernameList[userPos].ID;
                    wcout   << "*** FOUND CLIENT ***\n"
                            << "-> ID: " << userList[userPos].ID << '\n'
                            << "-> Name: " << userList[userPos].name << '\n'
                            << "-> Rented movies: " << userList[userPos].movies << '\n';
                }
                wcin.get();
            }
            // Executes if the user selects the "Exit" action. //
            else break;
        }
    }
    return 0;
}

wstring GetDate(bool add14Days){
    time_t rawtime;
    struct tm timeinfo;
    wchar_t buffer[20];

    time(&rawtime);
    if(add14Days) rawtime += 1209600;

    localtime_s(&timeinfo, &rawtime);

    wcsftime(buffer, 20, L"%Y-%m-%d", &timeinfo);

    return buffer;
}

void ClrScr(){
    #ifdef _WIN32
        // If on Windows OS
        std::system("cls");
    #else
        // Assuming POSIX OS
        std::system("clear");
    #endif
}

std::string GetDataDir(){
    using std::string;
    auto ValidDataDir = [](string path) -> bool{
        struct stat s;
        if((stat(path.c_str(), &s) == 0)){
            if(s.st_mode & S_IFDIR)
                return true;
        }
        return false;
    };

    char pBuf[256];
    size_t len = sizeof(pBuf);
    GetModuleFileName(NULL, pBuf, len);

    string path(pBuf);
    path.append("\\data");
    if(ValidDataDir(path)) return path;
    path.erase(path.find_last_of('\\'));

    for(int i = 0; i < 3; i++){
        path.erase(path.find_last_of('\\'));
        path.append("\\data");
        if(ValidDataDir(path)) return path;
        path.erase(path.find_last_of('\\'));
    }
    return "";
}