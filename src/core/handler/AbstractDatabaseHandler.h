/*
 * AbstractDatabaseHandler.h
 *
 *  Created on: 26 Jun 2015
 *      Author: Carmen
 */

#ifndef SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_
#define SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_



namespace repo{
	namespace core{
		namespace handler {
			class AbstractDatabaseHandler {
			public:
				/**
				 * A Deconstructor 
				 */
				~AbstractDatabaseHandler(){};

			protected:
				/**
				* Default constructor
				*/
				AbstractDatabaseHandler(){};
			};

		}
	}
}
#endif /* SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_ */
