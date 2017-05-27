/***********************************************************************************
Copyright (c) 2017, Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo,
Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include <ct/rbd/rbd.h>

#include <ct/models/HyQ/HyQ.h>
#include <ct/models/HyQ/codegen/HyQForwardZero.h>

using namespace ct::models::HyQ;

void timing(bool useContactModel)
{
	std::cout << std::boolalpha << "Using contact model: "<<useContactModel<<std::endl;

	typedef ct::rbd::FloatingBaseFDSystem<ct::rbd::HyQ::Dynamics, false> HyQSystem;
	std::shared_ptr<HyQSystem> hyqSys = std::shared_ptr<HyQSystem>(new HyQSystem);

	typedef ct::rbd::EEContactModel<typename HyQSystem::Kinematics> ContactModel;
	std::shared_ptr<ContactModel> contactModel = std::shared_ptr<ContactModel> (
			new ContactModel(
					5000.0,
					1000.0,
					100.0,
					100.0,
					-0.02,
					ContactModel::VELOCITY_SMOOTHING::SIGMOID,
					hyqSys->dynamics().kinematicsPtr())
	);
	if (useContactModel)
		hyqSys->setContactModel(contactModel);

	ct::models::HyQ::HyQForwardZero hyqForwardZero;

	static const size_t nTests = 100000;

	typedef typename HyQSystem::StateVector X;
	typedef typename HyQSystem::ControlVector U;
	typedef Eigen::Matrix<double, HyQSystem::STATE_DIM + HyQSystem::CONTROL_DIM + 1, 1> XUT;

	typedef HyQSystem::StateVector JacA;

	std::vector<X, Eigen::aligned_allocator<X>> x(nTests);
	std::vector<U, Eigen::aligned_allocator<U>> u(nTests);
	std::vector<double, Eigen::aligned_allocator<double>> t(nTests);
	std::vector<XUT, Eigen::aligned_allocator<XUT>> xut(nTests);

	std::vector<JacA, Eigen::aligned_allocator<JacA>> rawFd(nTests);
	std::vector<JacA, Eigen::aligned_allocator<JacA>> forwardZeroFd(nTests);

	std::cout << "running "<<nTests<<" tests"<<std::endl;
	for (size_t i=0; i<nTests; i++)
	{
		x[i].setRandom();
		u[i].setRandom();
		t[i] = i;

		xut[i] << x[i], u[i], t[i];
	}

	auto start = std::chrono::high_resolution_clock::now();
	for (size_t i=0; i<nTests; i++)
	{
		hyqSys->computeControlledDynamics(x[i], t[i], u[i], rawFd[i]);
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto diff = end - start;
	double msTotal = std::chrono::duration <double, std::micro> (diff).count()/1000.0;
	std::cout << "rawFd: " << msTotal << " ms. Average: " << msTotal/double(nTests) << " ms" << std::endl;

	start = std::chrono::high_resolution_clock::now();
	for (size_t i=0; i<nTests; i++)
	{
		forwardZeroFd[i] = hyqForwardZero(xut[i]);
	}
	end = std::chrono::high_resolution_clock::now();
	diff = end - start;
	msTotal = std::chrono::duration <double, std::micro> (diff).count()/1000.0;
	std::cout << "ForwardZero: " << msTotal << " ms. Average: " << msTotal/double(nTests) << " ms" << std::endl;


	bool failed = false;
	for (size_t i=0; i<nTests; i++)
	{
		if (!rawFd[i].isApprox(forwardZeroFd[i], 1e-10))
		{
			std::cout << "Raw FD and forwardZero not identical" << std::endl;
			std::cout << "raw FD: "<<std::endl << rawFd[i] << std::endl;
			std::cout << "forwardZeroFd A: "<<std::endl << forwardZeroFd[i] << std::endl<< std::endl<< std::endl;
			failed = true;
		}
		
		if (failed)
		{
			std::cout << "test failed, aborting"<<std::endl;
			break;
		}
	}
}


int main (int argc, char* argv[])
{
	timing(true);
	std::cout << std::endl;
	// timing(false);
	return 0;
}



