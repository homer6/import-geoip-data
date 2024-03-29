#include "geoip_importer.h"


using namespace std;

namespace Altumo{

    /**
    * Contructor. Creates the database connection.
    *
    */
    GeoipImporter::GeoipImporter(){

        //setup default values
            this->connector = new Connector();
            this->mysql_max_packet_size = this->connector->readMaxPacketSize() * 0.30;

    }


    /**
    * Destructor. Cleans up allocated memory.
    *
    */
    GeoipImporter::~GeoipImporter(){

        delete this->connector;

    }


    /**
    * Creates the empty "geo_ip_block" and "geo_ip_location" tables. These
    * tables must not exist first.
    *
    */
    int GeoipImporter::setProgramOptions( int argc, char** argv ){

        //handle the command line arguments
            boost::program_options::options_description desc("Options");
            desc.add_options()
                ( "help", "produce help message" )
                ( "locations-file", boost::program_options::value<string>(), "The locations csv eg. GeoIPCity-134-Location.csv" )
                ( "blocks-file", boost::program_options::value<string>(), "The blocks csv eg. GeoIPCity-134-Blocks.csv" )
            ;

            boost::program_options::variables_map variables_map;
            boost::program_options::store( boost::program_options::parse_command_line(argc, argv, desc), variables_map );
            boost::program_options::notify( variables_map );

            if( variables_map.count("help") ){
                cout << desc << "\n";
                return 1;
            }

            if( variables_map.count("locations-file") ){
                this->locations_filename = variables_map["locations-file"].as<string>();
            }else{
                cout << "The locations file is required.\n";
                cout << desc << "\n";
                return 1;
            }

            if( variables_map.count("blocks-file") ){
                this->blocks_filename = variables_map["blocks-file"].as<string>();
            }else{
                cout << "The blocks file is required.\n";
                cout << desc << "\n";
                return 1;
            }

            return 0;

    }


    /**
    * Drops the existing "geo_ip_block" and "geo_ip_location" tables.
    *
    */
    void GeoipImporter::clearExistingTables(){

        //drop the existing tables

        this->connector->executeStatement( "DROP TABLE IF EXISTS `geo_ip_location`", false );
        this->connector->executeStatement( "DROP TABLE IF EXISTS `geo_ip_block`", false );

    }


    /**
    * Creates the empty "geo_ip_block" and "geo_ip_location" tables. These
    * tables must not exist first.
    *
    */
    void GeoipImporter::createNewTables(){

        //build the new tables

            string current_query =
                "CREATE TABLE `geo_ip_location`("
                        "`id` INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,"
                        "`country` CHAR(2) NOT NULL,"
                        "`region` CHAR(2),"
                        "`city` VARCHAR(32),"
                        "`postal_code` VARCHAR(10),"
                        "`latitude` DECIMAL(18,12),"
                        "`longitude` DECIMAL(18,12),"
                        "`metro_code` SMALLINT,"
                        "`area_code` SMALLINT,"
                        "PRIMARY KEY (`id`),"
                        "INDEX `lat_long_index` ( `latitude`, `longitude` ),"
                        "INDEX `country_region_city_index` ( `country`, `region`, `city` ),"
                        "INDEX `postal_code_index` ( `postal_code` ),"
                        "INDEX `area_code_index` ( `area_code` )"
                ") ENGINE=MyISAM"
            ;
            this->connector->executeStatement( current_query, false );



            current_query =
                "CREATE TABLE `geo_ip_block`("
                        "`start_ip` INTEGER UNSIGNED NOT NULL,"
                        "`end_ip` INTEGER UNSIGNED NOT NULL,"
                        "`location_id` INTEGER UNSIGNED NOT NULL,"
                        "INDEX `ip_range_index` ( `start_ip`, `end_ip` ),"
                        "INDEX `location_index` ( `location_id` )"
                ") ENGINE=MyISAM"
            ;
            this->connector->executeStatement( current_query, false );

    }


    /**
    * Imports the GeoIPCity-134-Location.csv file, provided by MaxMind,
    * to a table called "geo_ip_location".
    *
    */
    void GeoipImporter::importLocationsFile(){

        //declare and initialize local variables
            string insert_query, locations_insert_query;
            bool first = true;
            int number_of_imported_records = 0;
            int number_of_skipped_records = 0;
            boost::cmatch result;
            string line;
            string metro_code;
            string area_code;

        //import the locations section
            ifstream locations_file( this->locations_filename.c_str() );
            const boost::regex locations_pattern( "(\\d+),\"(..)\",\"(..)\",\"(.*?)\",\"(.*?)\",(.*?),(.*?),(\\d*),(\\d*)" );
            number_of_imported_records = 0;
            first = true;
            locations_insert_query = "INSERT INTO geo_ip_location( id, country, region, city, postal_code, latitude, longitude, metro_code, area_code ) VALUES ";
            insert_query = locations_insert_query;

            while( !locations_file.eof() ){

                getline( locations_file, line );

                if( boost::regex_match(line.c_str(), result, locations_pattern) ){

                    number_of_imported_records++;
                    if( !first ){
                        insert_query += ",";
                    }else{
                        first = false;
                    }

                    metro_code = escapeString( result[8].str().c_str() );
                    if( metro_code.length() == 0 ){
                        metro_code = "NULL";
                    }

                    area_code = escapeString( result[8].str().c_str() );
                    if( area_code.length() == 0 ){
                        area_code = "NULL";
                    }

                    insert_query += " (" +
                                  escapeString( result[1].str().c_str() ) +
                                ", \"" +
                                  escapeString( result[2].str().c_str() ) +
                                "\", \"" +
                                  escapeString( result[3].str().c_str() ) +
                                "\", \"" +
                                  escapeString( result[4].str().c_str() ) +
                                "\", \"" +
                                  escapeString( result[5].str().c_str() ) +
                                "\", " +
                                  escapeString( result[6].str().c_str() ) +
                                ", " +
                                  escapeString( result[7].str().c_str() ) +
                                ", " +
                                  metro_code +
                                ", " +
                                  area_code +
                                ")";

                    if( insert_query.length() > this->mysql_max_packet_size ){
                        this->connector->executeStatement( insert_query );
                        insert_query = locations_insert_query;
                        first = true;
                    }
                }else{

                    number_of_skipped_records++;

                }

            }

            if( !first ){
                this->connector->executeStatement( insert_query );
            }

            first = true;
            cout << endl << number_of_imported_records << " locations records imported.";
            cout << endl << number_of_skipped_records << " locations records skipped." << endl;
            flush( cout );
            locations_file.close();

    }



    /**
    * Imports the GeoIPCity-134-Blocks.csv file, provided by MaxMind,
    * to a table called "geo_ip_block".
    *
    */
    void GeoipImporter::importBlocksFile(){

        //declare and initialize local variables
            string insert_query, blocks_insert_query;
            bool first = true;
            int number_of_imported_records = 0;
            int number_of_skipped_records = 0;
            boost::cmatch result;
            string line;

        //import the blocks section
            ifstream blocks_file( this->blocks_filename.c_str() );
            const boost::regex blocks_pattern( "\"(\\d+)\",\"(\\d+)\",\"(\\d+)\"" );
            blocks_insert_query = "INSERT INTO geo_ip_block( start_ip, end_ip, location_id ) VALUES ";
            insert_query = blocks_insert_query;

            while( !blocks_file.eof() ){

                getline( blocks_file, line, '\n' );

                if( boost::regex_match(line.c_str(), result, blocks_pattern) ){

                    number_of_imported_records++;

                    if( !first ){
                        insert_query += ",";
                    }else{
                        first = false;
                    }
                    insert_query += " (" +
                                  escapeString( result[1].str().c_str() ) +
                                ", " +
                                  escapeString( result[2].str().c_str() ) +
                                ", " +
                                  escapeString( result[3].str().c_str() ) +
                                ")";

                    if( insert_query.length() > this->mysql_max_packet_size ){
                        this->connector->executeStatement( insert_query );
                        insert_query = blocks_insert_query;
                        first = true;
                    }

                }else{

                    number_of_skipped_records++;

                }

            }

            if( !first ){
                this->connector->executeStatement( insert_query );
            }

            first = true;
            cout << endl << number_of_imported_records << " block records imported.";
            cout << endl << number_of_skipped_records << " block records skipped." << endl;
            flush( cout );
            blocks_file.close();

    }




    bool GeoipImporter::readyToClose(){

        if( this->connector->hasOpenConnections() ){
            return false;
        }else{
            return true;
        }

    }


}
