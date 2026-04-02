import { paths } from '@/config/paths';
import { Flex, Heading, Text, Button } from "@radix-ui/themes";

const NotFoundRoute = () => {
    return (
        <Flex direction="column" align="center" justify="center" gap="4" className="mt-52" minHeight="80vh">
            <Heading size="8" color="red">
                404 - Not Found
            </Heading>
            <Text size="4" color="gray">
                Sorry, the page you are looking for does not exist.
            </Text>
            <Button asChild>
                <a href={paths.home.getHref()}>
                    Go to Home
                </a>
            </Button>
        </Flex>
    );
};

export default NotFoundRoute;