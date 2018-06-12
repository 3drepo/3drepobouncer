/**
*  Copyright (C) 2016 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "repo_statistics_generator.h"
#include "../../core/model/bson/repo_bson_builder.h"
#include <fstream>

using namespace repo::manipulator;

StatisticsGenerator::StatisticsGenerator(
	repo::core::handler::AbstractDatabaseHandler *handler
	)
	: handler(handler)
{
}
StatisticsGenerator::~StatisticsGenerator()
{
}

void StatisticsGenerator::getUserList(const std::string &outputFilePath)
{
	auto users = handler->getAllFromCollectionTailable(REPO_ADMIN, REPO_SYSTEM_USERS);
	std::ofstream file;
	file.open(outputFilePath);
	file << "Username, Email, FirstName, Last Name, Date Activated, Country, Company, Position" << std::endl;
	std::cout << " looking through users..." << std::endl;
	for (const auto &user : users)
	{
		repo::core::model::RepoUser userBson(user);
		if (!userBson.getCustomDataBSON().isEmpty())
		{
			file << userBson.getUserName() << "," << userBson.getEmail() << "," << userBson.getFirstName() << "," << userBson.getLastName() << ",";
			if (auto createdAt = userBson.getUserCreatedAt()) {
				mongo::Date_t date(createdAt);
				file << date.toString();
			}
			file << ",";

			auto billing = userBson.getCustomDataBSON().getObjectField("billing");
			if (!billing.isEmpty())
			{
				auto billingInfo = billing.getObjectField("billingInfo");
				if (!billingInfo.isEmpty())
				{
					file << billingInfo.getStringField("countryCode") << "," << billingInfo.getStringField("company");
				}
			}
		}
		file << std::endl;
	}
	file.close();
}

static int64_t getTimeStamp(
	const int &year,
	const int &month)
{
	struct tm date;
	date.tm_year = year - 1900;
	date.tm_mon = month - 1;
	date.tm_mday = 1;
	date.tm_hour = date.tm_min = date.tm_sec = 0;
	date.tm_isdst = 0;

	auto timestamp = mktime(&date);
	mongo::Date_t mDate = mongo::Date_t(timestamp * 1000);
	return mDate.asInt64();
}

static void getYearMonthFromTimeStamp(
	const int64_t &timestamp,
	int &year,
	int &month)
{
	const time_t ts = timestamp / 1000;
	auto dateTime = gmtime(&ts);
	year = dateTime->tm_year + 1900;
	month = dateTime->tm_mon + 1;
}

static int64_t getFileSizeTotal(
	const std::vector<std::string>             &files,
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                          &dbName,
	const std::string                          &project
	)
{
	int64_t fileSize = 0;

	repo::core::model::RepoBSONBuilder builder;
	builder.appendArray("$in", files);
	auto criteria = BSON("filename" << builder.obj());

	auto filesInfo = handler->findAllByCriteria(dbName, project + ".history.files", criteria);

	for (const auto fileInfo : filesInfo)
	{
		fileSize += fileInfo.getIntField("length") / (1024 * 1024);
	}

	return fileSize;
}

static int64_t getTimestampOfFirstRevision(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                          &dbName,
	const std::string                          &project,
	std::map < int, std::map<int, int> >       &revisionsPerMonth,
	std::map < int, std::map<int, int> >       &fileSizesPerMonth)
{
	repo::core::model::RepoBSON criteria = BSON(REPO_NODE_REVISION_LABEL_INCOMPLETE << BSON("$exists" << false));
	auto bsons = handler->findAllByCriteria(dbName, project + ".history", criteria/*, REPO_NODE_REVISION_LABEL_TIMESTAMP*/);
	int64_t firstRevTS = -1;

	for (const auto &bson : bsons)
	{
		repo::core::model::RevisionNode revNode(bson);
		auto revTS = revNode.getTimestampInt64();

		if (revTS != -1)
		{
			if (firstRevTS == -1)
				firstRevTS = revTS;

			int year, month;
			getYearMonthFromTimeStamp(revTS, year, month);
			if (revisionsPerMonth.find(year) == revisionsPerMonth.end())
				revisionsPerMonth[year] = std::map<int, int>();
			if (revisionsPerMonth[year].find(month) == revisionsPerMonth[year].end())
				revisionsPerMonth[year][month] = 0;

			++revisionsPerMonth[year][month];

			auto orgFiles = revNode.getOrgFiles();

			if (fileSizesPerMonth.find(year) == fileSizesPerMonth.end())
				fileSizesPerMonth[year] = std::map<int, int>();
			if (fileSizesPerMonth[year].find(month) == fileSizesPerMonth[year].end())
				fileSizesPerMonth[year][month] = 0;

			fileSizesPerMonth[year][month] += getFileSizeTotal(orgFiles, handler, dbName, project);
		}
	}
	return firstRevTS;
}

static void getIssuesStatistics(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                          &dbName,
	const std::string                          &project,
	std::map < int, std::map<int, int> >       &issuesPerMonth,
	const int64_t                              &dbStartDate)
{
	auto bsons = handler->getAllFromCollectionTailable(dbName, project + ".issues");

	for (const auto &bson : bsons)
	{
		int64_t createdTS = -1;
		if (bson.hasField("created"))
			createdTS = bson.getField("created").Double();

		if (createdTS != -1)
		{
			if (dbStartDate != -1 && createdTS < dbStartDate)
				createdTS = dbStartDate;
			int year, month;
			getYearMonthFromTimeStamp(createdTS, year, month);
			if (issuesPerMonth.find(year) == issuesPerMonth.end())
				issuesPerMonth[year] = std::map<int, int>();
			if (issuesPerMonth[year].find(month) == issuesPerMonth[year].end())
				issuesPerMonth[year][month] = 0;

			++issuesPerMonth[year][month];
		}
	}
}


static void printMonthlyStatistic(
	const std::map <int, std::map<int, int>> &newProjectsPerMonth,
	const std::map <int, std::map<int, int>> &newRevisionsPerMonth,
	const std::map <int, std::map<int, int>> &newIssuesPerMonth,
	const std::map <int, std::map<int, int>> &fileSizesPerMonth,
	const std::string                        &prefix,
	std::ofstream							 &oFile) {


	repoInfo << "======== NEW PROJECTS PER MONTH =========";
	oFile << "New Projects per month (" << prefix << ")" << std::endl;
	oFile << "Year,Month,#New Projects,Total" << std::endl;
	int nProjects = 0;
	for (const auto yearEntry : newProjectsPerMonth)
	{
		auto year = yearEntry.first;
		for (const auto monthEntry : yearEntry.second)
		{
			repoInfo << "Year: " << year << "\tMonth: " << monthEntry.first << " \tProjects: " << monthEntry.second;
			nProjects += monthEntry.second;
			oFile << year << "," << monthEntry.first << "," << monthEntry.second << "," << nProjects << std::endl;
		}
	}

	int nRevisions = 0;
	repoInfo << "======== NEW REVISIONS PER MONTH   =========";
	oFile << "New revisions per month (" << prefix << ")" << std::endl;
	oFile << "Year,Month,#New Revisions,Total" << std::endl;
	for (const auto yearEntry : newRevisionsPerMonth)
	{
		auto year = yearEntry.first;
		for (const auto monthEntry : yearEntry.second)
		{
			repoInfo << "Year: " << year << "\tMonth: " << monthEntry.first << " \tRevisions: " << monthEntry.second;
			nRevisions += monthEntry.second;
			oFile << year << "," << monthEntry.first << "," << monthEntry.second << "," << nRevisions << std::endl;
		}
	}

	int nIssues = 0;
	repoInfo << "======== NEW ISSUES PER MONTH   =========";
	oFile << "New Issues per month (" << prefix << ")" << std::endl;
	oFile << "Year,Month,#New Issues,Total" << std::endl;
	for (const auto yearEntry : newIssuesPerMonth)
	{
		auto year = yearEntry.first;
		for (const auto monthEntry : yearEntry.second)
		{
			repoInfo << "Year: " << year << "\tMonth: " << monthEntry.first << " \tIssues: " << monthEntry.second;
			nIssues += monthEntry.second;
			oFile << year << "," << monthEntry.first << "," << monthEntry.second << "," << nIssues << std::endl;
		}
	}

	repoInfo << "======== NEW FILES SIZE PER MONTH  =========";
	int64_t fSize = 0;
	oFile << "New file size per month (" << prefix << ")" << std::endl;
	oFile << "Year,Month,File Size(MiB),Total" << std::endl;
	for (const auto yearEntry : fileSizesPerMonth)
	{
		auto year = yearEntry.first;
		for (const auto monthEntry : yearEntry.second)
		{
			fSize += monthEntry.second;
			repoInfo << "Year: " << year << "\tMonth: " << monthEntry.first << " \tSize(MiB): " << monthEntry.second;

			oFile << year << "," << monthEntry.first << "," << monthEntry.second << "," << fSize << std::endl;
		}
	}

}


static void getProjectsStatistics(
	const std::list<std::string>              &databases,
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::unordered_map<std::string, int64_t>  &userStartDate,
	std::ofstream							  &oFile,
	const std::set<std::string>					&enterpriseAccounts,
	const std::set<std::string>					&discretionaryAccounts
	)
{
	repoInfo << "Getting database projects...";
	auto projects = handler->getDatabasesWithProjects(databases);
	repoInfo << "done";
	std::map < int, std::map<int, int> > newProjectsPerMonth;
	std::map < int, std::map<int, int> > newRevisionsPerMonth;
	std::map < int, std::map<int, int> > newIssuesPerMonth;
	std::map < int, std::map<int, int> > fileSizesPerMonth;
	int count = 0;
	for (const auto &dbEntry : projects)
	{
		auto dbName = dbEntry.first;
		repoInfo << "Going through DB: " << dbName << " #models: " << dbEntry.second.size();
		int64_t dbStartDate = -1;
		if (userStartDate.find(dbName) != userStartDate.end())
			dbStartDate = userStartDate[dbName];

		const bool isEnterprise = enterpriseAccounts.find(dbName) != enterpriseAccounts.end();
		const bool isDemo = discretionaryAccounts.find(dbName) != discretionaryAccounts.end();
		const bool shouldPrint = isEnterprise || isDemo;

		std::map < int, std::map<int, int> > *_newProjectsPerMonth;
		std::map < int, std::map<int, int> > *_newRevisionsPerMonth;
		std::map < int, std::map<int, int> > *_newIssuesPerMonth;
		std::map < int, std::map<int, int> > *_fileSizesPerMonth;

		if (shouldPrint){
			std::map < int, std::map<int, int> > accNewProjectsPerMonth;
			std::map < int, std::map<int, int> > accNewRevisionsPerMonth;
			std::map < int, std::map<int, int> > accNewIssuesPerMonth;
			std::map < int, std::map<int, int> > accFileSizesPerMonth;

			_newProjectsPerMonth = &accNewProjectsPerMonth;
			_newRevisionsPerMonth = &accNewRevisionsPerMonth;
			_newIssuesPerMonth = &accNewIssuesPerMonth;
			_fileSizesPerMonth = &accFileSizesPerMonth;
		} else
		{
			_newProjectsPerMonth = &newProjectsPerMonth;
			_newRevisionsPerMonth = &newRevisionsPerMonth;
			_newIssuesPerMonth = &newIssuesPerMonth;
			_fileSizesPerMonth = &fileSizesPerMonth;
		}
		

		for (const auto project : dbEntry.second)
		{
			auto time = getTimestampOfFirstRevision(handler, dbName, project, *_newRevisionsPerMonth, *_fileSizesPerMonth);
			if (time != -1)
			{
				int year = 0, month = 0;
				getYearMonthFromTimeStamp(time, year, month);
				if (_newProjectsPerMonth->find(year) == _newProjectsPerMonth->end())
					(*_newProjectsPerMonth)[year] = std::map<int, int>();
				if (*_newProjectsPerMonth[year].find(month) == *_newProjectsPerMonth[year].end())
					(*_newProjectsPerMonth)[year][month] = 0;

				++(*_newProjectsPerMonth)[year][month];
			}

			getIssuesStatistics(handler, dbName, project, *_newIssuesPerMonth, dbStartDate);			
		}

		if (shouldPrint)
		{
			printMonthlyStatistic(
				*_newProjectsPerMonth,
				*_newRevisionsPerMonth,
				*_newIssuesPerMonth,
				*_fileSizesPerMonth,
				dbName + (isEnterprise ? " (E)" : " (D)"),
				oFile);
		}
	}

	printMonthlyStatistic(
		newProjectsPerMonth,
		newRevisionsPerMonth,
		newIssuesPerMonth,
		fileSizesPerMonth,
		"Other Users",
		oFile);
}
static void getNewPaidUsersPerMonth(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::list<std::string>              &databases,
	std::ofstream							  &oFile)
{
	std::map<int, std::map<int, int>> paidUsers;
	for (const auto &db : databases)
	{
		auto invoices = handler->getAllFromCollectionTailable(db, "invoices");
		for (const auto invoice : invoices)
		{
			std::string state = invoice.getStringField("state");
			if (invoice.hasField("periodStart") && (state == "complete"))
			{
				auto start = invoice.getTimeStampField("periodStart");
				if (start != -1)
				{
					auto nLicense = invoice.getObjectField("items").nFields();
					int year, month;
					getYearMonthFromTimeStamp(start, year, month);
					if (paidUsers.find(year) == paidUsers.end())
						paidUsers[year] = std::map<int, int>();
					if (paidUsers[year].find(month) == paidUsers[year].end())
						paidUsers[year][month] = 0;
					repoInfo << " nLicenses : " << nLicense << ": " << db;
					paidUsers[year][month] += nLicense;
				}
			}
		}
	}

	repoInfo << "======== PAID USERS PER MONTH =========";
	int64_t nUsers = 0;
	oFile << "Paid users per month" << std::endl;
	oFile << "Year,Month,#Users" << std::endl;
	for (const auto yearEntry : paidUsers)
	{
		auto year = yearEntry.first;
		for (const auto monthEntry : yearEntry.second)
		{
			nUsers += monthEntry.second;
			repoInfo << "Year: " << year << "\tMonth: " << monthEntry.first << " \t#Users: " << monthEntry.second;

			oFile << year << "," << monthEntry.first << "," << monthEntry.second << "," << nUsers << std::endl;
		}
	}
}

static uint64_t getNewUsersWithinDuration(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const int64_t                             &from,
	const int64_t                             &to,
	std::unordered_map<std::string, int64_t>  &userStartDate)
{
	repo::core::model::RepoBSONBuilder timeRangeBuilder;
	timeRangeBuilder.appendTime("$lt", to);
	timeRangeBuilder.appendTime("$gt", from);

	repo::core::model::RepoBSON planInfo;

	auto subscriptionCriteria = BSON("$elemMatch" << planInfo);
	repo::core::model::RepoBSON criteria = BSON("customData.createdAt" << timeRangeBuilder.obj());
	auto users = handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_USERS, criteria);
	std::vector<repo::core::model::RepoBSON> filteredUsers;
	for (const auto userBson : users)
	{
		repo::core::model::RepoUser user(userBson);
		auto custom = user.getCustomDataBSON();
		if (custom.hasField("inactive") && custom.getBoolField("inactive")) continue;
		if (auto createdAt = user.getUserCreatedAt()) {
			mongo::Date_t date(createdAt);
			userStartDate[user.getUserName()] = createdAt;
		}		
		filteredUsers.push_back(user);
	}

	return filteredUsers.size();
}

static void getNewUsersPerMonth(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::unordered_map<std::string, int64_t>  &userStartDate,
	std::ofstream							  &oFile)
{
	int month = 8;
	int year = 2016;

	time_t rawTime;
	time(&rawTime);
	auto currTime = gmtime(&rawTime);
	int maxYear = currTime->tm_year + 1900;
	int maxMonth = currTime->tm_mon + 1;

	int nUsers = 0;
	oFile << "Number of new users per month" << std::endl;
	oFile << "Year,Month,Users,Total" << std::endl;
	while (maxYear > year || (maxYear == year && maxMonth >= month))
	{
		int nextMonth = month == 12 ? 1 : month + 1;
		int nextYear = nextMonth == 1 ? year + 1 : year;

		auto from = getTimeStamp(year, month);
		auto to = getTimeStamp(nextYear, nextMonth);
		auto users = getNewUsersWithinDuration(handler, from, to, userStartDate);
		nUsers += users;
		repoInfo << "Year: " << year << "\tMonth: " << month << " \tUsers: " << users;
		oFile << year << "," << month << "," << users << "," << nUsers << std::endl;

		year = nextYear;
		month = nextMonth;
	}
}


std::set<std::string> printUsercountStatistics(
	const std::vector<repo::core::model::RepoBSON>	&userList,
	repo::core::handler::AbstractDatabaseHandler	*handler,
	std::ofstream									&oFile) {


	std::set <std::string> accountList;
	int totalCount = 0;
	for (const auto res : userList) {
		auto user = repo::core::model::RepoUser(res);
		accountList.insert(user.getUserName());
		int userCount = 0;
		auto subs = user.getSubscriptionInfo();
		if (repo::core::model::RepoBSON::getCurrentTimestamp() > subs.enterprise.expiryDate)
		{
			userCount += handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_USERS, BSON("roles.db" << user.getUserName())).size();
		}

		totalCount += userCount;
		repoInfo << user.getUserName() << ", " << userCount << "," << totalCount;
		oFile << user.getUserName() << ", " << userCount << "," << totalCount << std::endl;
	}

	return accountList;

}


std::set<std::string> getPaidForUsersCount(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::ofstream							  &oFile)
{

	oFile << "Enterprise Account, User Count , Total" << std::endl;

	auto results = handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_USERS,
		BSON(
			"customData.billing.subscriptions.enterprise" << BSON("$exists" << true)
		));
	return printUsercountStatistics(results, handler, oFile);
}

std::set<std::string> getDemoUsersCount(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::ofstream							  &oFile)
{

	oFile << "Discretionary Account, User Count , Total" << std::endl;
	auto results = handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_USERS,
		BSON(
			"customData.billing.subscriptions.discretionary" << BSON("$exists" << true)
		));

	return printUsercountStatistics(results, handler, oFile);
}
void StatisticsGenerator::getDatabaseStatistics(
	const std::string &outputFilePath)
{
	std::ofstream oFile;
	oFile.open(outputFilePath);
	if (oFile.is_open())
	{
		std::unordered_map<std::string, int64_t>  userStartDate;
		repoInfo << "======== NEW USERS PER MONTH ==========";
		auto databases = handler->getDatabases();
		getNewUsersPerMonth(handler, userStartDate, oFile);
		getNewPaidUsersPerMonth(handler, databases, oFile);
		auto enterpriseDBs = getPaidForUsersCount(handler, oFile);
		auto discretionaryDBs = getDemoUsersCount(handler, oFile);
		getProjectsStatistics(databases, handler, userStartDate, oFile, enterpriseDBs, discretionaryDBs);

	}
	else
	{
		repoError << "Failed to open output file:  " << outputFilePath;
	}
	oFile.close();
}
