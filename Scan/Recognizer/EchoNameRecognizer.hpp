//
// Created by EMCJava on 7/15/2024.
//

#pragma once

#include "RecognizerBase.hpp"

class EchoNameRecognizer : public RecognizerBase<EchoNameRecognizer>
{
public:
    explicit EchoNameRecognizer( const std::filesystem::path& EchoNameFolder = "data/names" )
    {
        Templates = std::filesystem::directory_iterator { EchoNameFolder }
            | std::views::enumerate
            | std::views::transform( []( const auto& Index_DirEntry ) {
                        const auto& path = std::get<1>( Index_DirEntry ).path( );
                        spdlog::info( "Loading template: {} ", path.stem( ).string( ) );
                        return Template {
                            (int32_t) std::get<0>( Index_DirEntry ),
                            std::filesystem::absolute( path ).string( ),
                            cv::Scalar(
                                100 + rand( ) % ( 255 - 100 ),
                                100 + rand( ) % ( 255 - 100 ),
                                100 + rand( ) % ( 255 - 100 ) ),
                            215 };
                    } )
            | std::ranges::to<std::vector>( );

        Initialize( );
    }

    [[nodiscard]] const auto& GetTemplates( ) const
    {
        return Templates;
    }

private:
    std::vector<Template> Templates;
};
