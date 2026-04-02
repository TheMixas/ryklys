import { Outlet } from 'react-router';
import {Flex} from "@radix-ui/themes";
import { Navbar } from "@/components/ui/navbar/navbar";


export const ErrorBoundary = () => {
    return <div>Something went wrong!</div>;
};

const AppRoot = () => {
    return (
        <Flex direction="column" style={{minHeight: "100vh"}}>
            <Navbar />
            <Flex style={{flex: 1}}>
                <Outlet />
            </Flex>
        </Flex>
    );
};

export default AppRoot;