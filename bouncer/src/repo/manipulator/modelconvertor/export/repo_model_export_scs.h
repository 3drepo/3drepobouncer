
#pragma once

#include <repo/manipulator/modelconvertor/export/repo_model_export_web.h>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class ScsExport : public WebModelExport
			{
			public:
				ScsExport(repo::core::model::RepoScene* scene);
				repo::lib::repo_web_buffers_t getAllFilesExportedAsBuffer() const;

			private:
				repo::lib::repo_web_buffers_t buffers;
			};
		}
	}
}