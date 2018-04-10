/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// vtkAddon includes
#include "vtkPersonInformation.h"

using namespace std;

#define ASSERT_TRUE(returnValue) \
  if (returnValue == false) { \
    std::cerr <<  __FILE__ << ":" << __LINE__  << std::endl; \
    return EXIT_FAILURE; \
  }

bool isEqual(const std::string& string1, const std::string& string2) {
  return string1.compare(string2) == 0;
}

int vtkPersonInformationTest1(int, char**)
{
  vtkPersonInformation* UserInformation = vtkPersonInformation::New();

  string name = "John Doe";
  string login = "foobar";
  string email = "valid.email@somewhere.com";
  string organization = "BWH";
  string organizationRole = "Referring";
  string procedureRole = "Some Procedure Role";

  ASSERT_TRUE(UserInformation->ReadFromString(""));

  ASSERT_TRUE(UserInformation->SetName(name));
  ASSERT_TRUE(UserInformation->SetLogin(login));
  ASSERT_TRUE(UserInformation->SetEmail(email));
  ASSERT_TRUE(!UserInformation->SetEmail("invalid email"));
  ASSERT_TRUE(UserInformation->SetOrganization(organization));
  ASSERT_TRUE(UserInformation->SetOrganizationRole(organizationRole));
  ASSERT_TRUE(UserInformation->SetProcedureRole(procedureRole));

  ASSERT_TRUE(isEqual(name, UserInformation->GetName()));
  ASSERT_TRUE(isEqual(login, UserInformation->GetLogin()));
  ASSERT_TRUE(isEqual(email, UserInformation->GetEmail()));
  ASSERT_TRUE(isEqual(organization, UserInformation->GetOrganization()));
  ASSERT_TRUE(isEqual(organizationRole, UserInformation->GetOrganizationRole()));
  ASSERT_TRUE(isEqual(procedureRole, UserInformation->GetProcedureRole()));

  std::string output = "Email:valid.email@somewhere.com;Login:foobar;"
                       "Name:John Doe;Organization:BWH;"
                       "OrganizationRole:Referring;"
                       "ProcedureRole:Some Procedure Role";
  ASSERT_TRUE(isEqual(output, UserInformation->WriteToString()))

  ASSERT_TRUE(UserInformation->SetName(""));
  ASSERT_TRUE(UserInformation->SetLogin(""));
  ASSERT_TRUE(UserInformation->SetEmail(""));
  ASSERT_TRUE(UserInformation->SetOrganization(""));
  ASSERT_TRUE(UserInformation->SetOrganizationRole(""));
  ASSERT_TRUE(UserInformation->SetProcedureRole(""));

  ASSERT_TRUE(isEqual("", UserInformation->WriteToString()))

  UserInformation->Delete();

  return EXIT_SUCCESS;
}
