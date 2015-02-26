// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once
#include <Steps/Step.hpp>
#include <Steps/Step_Export.hpp>

#include <Common/Log.hpp>

namespace HighQueue
{
    namespace Steps
    {
        class Steps_Export Tee: public Step
        {
        public:
            /// @brief hard code the line width.
            static const size_t bytesPerLine = 16;

            /// @brief construct
            Tee();

            virtual bool configureParameter(const std::string & key, const ConfigurationNode & configuration);
            virtual void start();

            /// @brief Attach an ostream.  If none, don't dump the data.
            void attachOutputStream(std::ostream * outputStream);

            // implement Step methods
            virtual void handle(Message & message);
            virtual void finish();
            virtual std::ostream & usage(std::ostream & out) const;
        private:
            void hexDump(byte_t * message, size_t size);
        private:
            std::string outputName_;
            std::ostream * out_;
            std::ofstream outfile_;
        };

   }
}